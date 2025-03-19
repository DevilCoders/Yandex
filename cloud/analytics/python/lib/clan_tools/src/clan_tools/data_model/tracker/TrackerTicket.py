from typing import List, Optional
from dataclasses import dataclass


@dataclass
class TrackerTicket:
    queue: str
    summary: str
    description: str
    type: int = 2
    assignee: Optional[str] = None
    followers: Optional[List[str]] = None
    tags: Optional[List[str]] = None
