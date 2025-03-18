import antirobot.cbb.cbb_django.cbb.library.db as db
from sqlalchemy import Index

from . import HistoryIPPrototype


class HistoryIPV4(db.history.Base, HistoryIPPrototype):
    version = 4
    __tablename__ = "ipv4"
    __table_args__ = (
        Index("ix_unblocked_at_4", "unblocked_at"),
        Index("ix_xtra_start_4", "xtra_large", "rng_start"),
    )
