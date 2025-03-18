import antirobot.cbb.cbb_django.cbb.library.db as db
from sqlalchemy import Column, String

from . import HistoryLinePrototype


class LimboTXT(db.main.Base, HistoryLinePrototype):
    version = "txt"
    __tablename__ = "limbo_txt"

    txt = Column(String(1024), default="", nullable=False)
    value_key = "txt"
