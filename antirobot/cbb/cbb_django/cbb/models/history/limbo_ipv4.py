import antirobot.cbb.cbb_django.cbb.library.db as db

from . import HistoryIPPrototype


class LimboIPV4(db.main.Base, HistoryIPPrototype):
    version = 4
    __tablename__ = "limbo_ipv4"
