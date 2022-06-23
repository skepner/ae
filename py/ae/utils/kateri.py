import sys, os, asyncio, subprocess
from pathlib import Path
from typing import Optional

import ae_backend

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

    def send_ace(self, filename: Path):
        self._send(b"CHRT", subprocess.check_output(["decat", str(filename)]))

    def send_chart(self, chart: ae_backend.chart_v3.Chart):
        self._send(b"CHRT", chart.export())

    def set_style(self, style: str):
        self.send_command({"C": "set_style", "style": style})

    def pdf(self, filename: str|Path, style: str = None, width: float = 800.0, open: bool = False):
        if style:
            self.set_style(style=style)
        self.send_command({"C": "pdf", "width": width})
        self.expected.append({"C": "PDFB", "filename": filename, "open": open})

    def quit(self):
        self.send_command({"C": "quit"})

    def send_command(self, command: dict[str, object]):
        import json
        self._send(b"COMD", json.dumps(command).encode("utf-8"))

    def _send(self, data_code: bytes, data: bytes):
        if not self.writer:
            raise RuntimeError("communicator is not connected")
        self.writer.write(data_code)
        self.writer.write(len(data).to_bytes(4, byteorder=sys.byteorder))
        self.writer.write(data)
        # sent data must contain number of bytes divisible by 4
        if last_chunk := len(data) % 4:
            self.writer.write(b"\x00" * (4 - last_chunk))

    async def connected(self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter):
        self.writer = writer
        while (request := (await reader.read(4)).decode('utf8').strip()) not in ["", "QUIT"]: # empty request means broken pipe
            # print(f">>>> received from kateri: {request}", file=sys.stderr)
            if request == "HELO":
                pass
            elif request == "PDFB":
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

    def _process_expected_request(self, expected: dict[str, object], data: bytes):
        if expected["C"] == "PDFB":
            print(f">>> [kateri.Communicator] writing pdf to {expected['filename']}", file=sys.stderr)
            with Path(expected["filename"]).open("wb") as output:
                output.write(data)
            if expected.get("open"):
                subprocess.call(["open", expected["filename"]])
        else:
            print(f">> [kateri.Communicator] not implemented processing for expected {expected}", file=sys.stderr)

# ----------------------------------------------------------------------

communicator = Communicator()

# ======================================================================
