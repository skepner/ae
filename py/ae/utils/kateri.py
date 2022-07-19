import sys, os, asyncio, subprocess, json
from pathlib import Path
from typing import Optional, Callable, Any

import ae_backend.chart_v3

# ======================================================================

KATERI_EXE = "kateri"

# ----------------------------------------------------------------------

class KateriTask:

    def __init__(self):
        self.kateri = None

    async def start(self, socket_name: str, **ignored):
        try:
            self.kateri = await asyncio.create_subprocess_exec(KATERI_EXE, "--socket", socket_name)
            print(f">>> [Kateri] started", file=sys.stderr)
            retcode = await self.kateri.wait()
            print(f">>> [Kateri] finished with code {retcode}", file=sys.stderr)
        except asyncio.CancelledError:
            self.kateri.terminate()
            self.kateri = None

    def running(self) -> bool:
        return self.kateri is not None

    def name(self):
        return self.__class__

# ----------------------------------------------------------------------

class SocketServerTask:

    def __init__(self):
        self.socket_server = None
        self.communicator = None

    async def start(self, socket_name: str, **ignored):
        global communicator
        self.socket_server = await asyncio.start_unix_server(communicator.connected, socket_name)
        print(f">>> [server-for-kateri] started pid: {os.getpid()} socket: {socket_name}", file=sys.stderr)
        await self.socket_server.serve_forever()
        self.socket_server = None
        print(f">>> [server-for-kateri] completed", file=sys.stderr)

    def running(self) -> bool:
        global communicator
        return self.socket_server is not None and communicator is not None and communicator.is_connected()

    def name(self):
        return self.__class__

# ----------------------------------------------------------------------

class Communicator:

    def __init__(self):
        self.writer: asyncio.StreamWriter = None
        self.expected = []
        self._command_id: int = 0

    def send_ace(self, filename: Path):
        self._send(b"CHRT", subprocess.check_output(["decat", str(filename)]))

    def send_chart(self, chart: ae_backend.chart_v3.Chart):
        self._send(b"CHRT", chart.export())

    def set_style(self, style: str):
        self.send_command({"C": "set_style", "style": style})

    def export_to_legacy(self, style: Optional[str] = None):
        if style:
            self.send_command({"C": "set_style", "style": style})
        self.send_command({"C": "export_to_legacy"})

    async def get_chart(self) -> ae_backend.chart_v3.Chart:
        futu = asyncio.get_running_loop().create_future()
        self.send_command_expect(command={"C": "get_chart"}, expect={"C": "CHRT", "future": futu})
        return await futu

    async def get_viewport(self) -> dict:
        futu = asyncio.get_running_loop().create_future()
        self.send_command_expect(command={"C": "get_viewport"}, expect={"C": "JSON", "future": futu})
        viewport = await futu
        return viewport

    async def get_pdf(self, style: str = None, width: float = 800.0) -> bytes:
        if style:
            self.set_style(style=style)
        futu = asyncio.get_running_loop().create_future()
        self.send_command_expect(command={"C": "pdf", "width": width}, expect={"C": "PDFB", "future": futu})
        return await futu

    # def pdf(self, filename: str|Path, style: str = None, width: float = 800.0, open: bool = False):
    #     if style:
    #         self.set_style(style=style)
    #     self.send_command_expect(command={"C": "pdf", "width": width}, expect={"C": "PDFB", "filename": filename, "open": open})

    def quit(self):
        self.send_command({"C": "quit"})

    def send_command_expect(self, command: dict[str, Any], expect: dict[str, Any]):
        command_id = self.send_command(command=command)
        self.expected.append({**expect, "_id": command_id})

    def send_command(self, command: dict[str, Any]) -> int:
        "return sent command id"
        import json
        # print(f">>>> send to kateri \"{json.dumps(command)}\"", file=sys.stderr)
        self._command_id += 1
        self._send(b"COMD", json.dumps({**command, "_id": self._command_id}).encode("utf-8"))
        return self._command_id

    def _send(self, data_code: bytes, data: bytes):
        if not self.writer:
            raise RuntimeError("communicator is not connected")
        self.writer.write(data_code)
        self.writer.write(len(data).to_bytes(4, byteorder=sys.byteorder))
        self.writer.write(data)
        # sent data must contain number of bytes divisible by 4
        if last_chunk := len(data) % 4:
            self.writer.write(b"\x00" * (4 - last_chunk))
        # if data_code == b"COMD":
        #     print(f">>>> sent data {data_code} {len(data)} bytes {data} with padding {4 - last_chunk}", file=sys.stderr)

    async def connected(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter):
        self.writer = writer
        while (request := (await reader.read(4)).decode('utf8').strip()) not in ["", "QUIT"]: # empty request means broken pipe
            # print(f">>>> received from kateri: {request}", file=sys.stderr)
            if request == "HELO":
                pass
            elif request in ["PDFB", "CHRT", "JSON"]:
                payload_length = int.from_bytes(await reader.read(4), byteorder=sys.byteorder)
                # print(f">>>> [kateri.Communicator] {request} {payload_length} bytes", file=sys.stderr)
                self._process_expected(request, await self._read_with_padding(reader=reader, payload_length=payload_length))
            else:
                print(f">> [kateri.Communicator] unrecognized request \"{request}\"", file=sys.stderr)
            await writer.drain()
        print(f">>>> kateri: quit", file=sys.stderr)
        writer.close()

    def is_connected(self) -> bool:
        return self.writer is not None

    async def _read_with_padding(self, reader: asyncio.StreamReader, payload_length: int):
        data = bytes()
        left = payload_length
        while left > 0:
            chunk = await reader.read(left)
            # print(f">>>> [kateri.Communicator] read {len(chunk)} of {left}", file=sys.stderr)
            data += chunk
            left -= len(chunk)
        if (padding := 4 - payload_length % 4) != 4:
            await reader.read(padding)
        return data

    def _process_expected(self, code: str, data: bytes):
        for no, en in enumerate(self.expected):
            if en["C"] == code:
                self._process_expected_request(expected=en, data=data)
                self.expected[no:no+1] = []
                break;

    def _process_expected_request(self, expected: dict[str, Any], data: bytes):
        if expected["C"] == "PDFB":
            print(f">>> [kateri.Communicator] receiving pdf ({len(data)} bytes)", file=sys.stderr)
            if futu := expected.get("future"):
                futu.set_result(data)
        elif expected["C"] == "CHRT":
            print(f">>> [kateri.Communicator] receiving chart ({len(data)} bytes)", file=sys.stderr)
            if futu := expected.get("future"):
                futu.set_result(ae_backend.chart_v3.chart_from_json(data))
        elif expected["C"] == "JSON":
            print(f">>> [kateri.Communicator] receiving json ({len(data)} bytes)", file=sys.stderr)
            if futu := expected.get("future"):
                futu.set_result(json.loads(data))
        else:
            print(f">> [kateri.Communicator] not implemented processing for expected {expected}", file=sys.stderr)

# ----------------------------------------------------------------------

communicator = Communicator()

# ======================================================================
