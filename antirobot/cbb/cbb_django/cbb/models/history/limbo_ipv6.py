import antirobot.cbb.cbb_django.cbb.library.db as db

from . import HistoryIPPrototype


class LimboIPV6(db.main.Base, HistoryIPPrototype):
    version = 6
    __tablename__ = "limbo_ipv6"
