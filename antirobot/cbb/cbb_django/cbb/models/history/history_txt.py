import antirobot.cbb.cbb_django.cbb.library.db as db
from sqlalchemy import Column, Index, String

from . import HistoryLinePrototype


class HistoryTXT(db.history.Base, HistoryLinePrototype):
    version = "txt"
    __tablename__ = "txt"
    __table_args__ = (
        Index("ix_unblocked_at_txt", "unblocked_at"),
    )

    txt = Column(String(1024), default="", nullable=False, index=True)
    value_key = "txt"
