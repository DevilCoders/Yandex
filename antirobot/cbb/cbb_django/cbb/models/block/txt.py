import antirobot.cbb.cbb_django.cbb.library.db as db
from sqlalchemy import Column, Index, String, UniqueConstraint, event

from . import BlockPrototype, BlockLinePrototype


class BlockTXT(db.main.Base, BlockLinePrototype):
    version = "txt"

    __tablename__ = "cbb_range_txt"
    __table_args__ = (
        UniqueConstraint("group_id", "rng_txt"),
        Index("ix_rng_txt", "rng_txt"),
    )

    rng_txt = Column(String(1024), nullable=False, default="", index=True)
    value_key = "rng_txt"

    def __repr__(self):
        return f"<BlockTXT group_id: {self.group_id}, rng_txt: {self.rng_txt}>"


# ToDo: all same event to prototype
@event.listens_for(BlockPrototype, "before_insert", propagate=True)
def unquote_block_descr_column(mapper, connection, target):
    if target.block_descr:
        target.block_descr = target.block_descr. \
            replace(""", "").replace(""", "").strip()
