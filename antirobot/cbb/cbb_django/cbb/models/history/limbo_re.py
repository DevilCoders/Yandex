import antirobot.cbb.cbb_django.cbb.library.db as db
from sqlalchemy import Column, String

from . import HistoryLinePrototype


class LimboRE(db.main.Base, HistoryLinePrototype):
    version = "re"
    __tablename__ = "limbo_re"

    re = Column(String(1024), default="", nullable=False)
    value_key = "re"
