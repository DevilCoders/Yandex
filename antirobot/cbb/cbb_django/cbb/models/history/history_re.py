import antirobot.cbb.cbb_django.cbb.library.db as db
from sqlalchemy import Column, Index, String

from . import HistoryLinePrototype


class HistoryRE(db.history.Base, HistoryLinePrototype):
    version = "re"
    __tablename__ = "re"
    __table_args__ = (
        Index("ix_unblocked_at_re", "unblocked_at"),
    )

    re = Column(String(1024), default="", nullable=False, index=True)
    value_key = "re"
