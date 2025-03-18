import antirobot.cbb.cbb_django.cbb.library.db as db
from sqlalchemy import PrimaryKeyConstraint

from . import BlockIPPrototype


class BlockIPV4(db.main.Base, BlockIPPrototype):
    version = 4
    __tablename__ = "cbb_range_ipv4"
    __table_args__ = (
        PrimaryKeyConstraint("group_id", "rng_start", "rng_end", "is_exception"),
    )
