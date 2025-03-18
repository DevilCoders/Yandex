import antirobot.cbb.cbb_django.cbb.library.db as db
from sqlalchemy import Index

from . import HistoryIPPrototype


class HistoryIPV6(db.history.Base, HistoryIPPrototype):
    version = 6
    __tablename__ = "ipv6"
    __table_args__ = (
        Index("ix_unblocked_at_6", "unblocked_at"),
        Index("ix_xtra_start_6", "xtra_large", "rng_start"),
    )
