from datetime import datetime
from typing import NamedTuple


class Cluster(NamedTuple):
    cid: str
    type: str
    name: str
    create_at: datetime


class TaskStatus(NamedTuple):
    done: bool
    failed: bool
