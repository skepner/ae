from pathlib import Path
from dataclasses import dataclass

import ae_backend

# ======================================================================

@dataclass
class Message:
    field: str
    value: str
    message: str
    message_raw: ae_backend.Message
    filename: Path
    line_no: int

# ======================================================================
