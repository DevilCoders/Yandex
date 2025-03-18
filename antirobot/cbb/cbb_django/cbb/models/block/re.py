import antirobot.cbb.cbb_django.cbb.library.db as db
from sqlalchemy import Column, Index, String, UniqueConstraint

from . import BlockLinePrototype


class BlockRE(db.main.Base, BlockLinePrototype):
    version = "re"

    __tablename__ = "cbb_range_re"
    __table_args__ = (
        UniqueConstraint("group_id", "rng_re"),
        Index("ix_rng_re", "rng_re"),
    )

    rng_re = Column(String(1024), nullable=False, default="", index=True)
    value_key = "rng_re"

    def __repr__(self):
        return f"<BlockRE group_id: {self.group_id}, rng_re: {self.rng_re}>"
