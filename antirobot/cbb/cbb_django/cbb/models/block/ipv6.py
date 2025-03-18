import antirobot.cbb.cbb_django.cbb.library.db as db
from sqlalchemy import PrimaryKeyConstraint

from . import BlockIPPrototype


class BlockIPV6(db.main.Base, BlockIPPrototype):
    version = 6
    __tablename__ = "cbb_range_ipv6"
    __table_args__ = (
        PrimaryKeyConstraint("group_id", "rng_start", "rng_end", "is_exception"),
    )
