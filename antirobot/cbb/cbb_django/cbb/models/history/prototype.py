from datetime import datetime

import antirobot.cbb.cbb_django.cbb.library.db as db
import antirobot.cbb.cbb_django.cbb.models as models
from sqlalchemy import (BigInteger, Column, DateTime, Integer, String, Text, event)


class HistoryPrototype(object):
    """
    Base history prototype for ipv4, ipv6, txt.
    It defines all common columns for the tables.
    """
    id = Column(BigInteger, autoincrement=True, primary_key=True)
    group_id = Column(Integer, default=0, nullable=False)
    # columns about block: who? when? why?
    blocked_by = Column(String(64), default="", nullable=False)
    blocked_at = Column(DateTime, default=datetime.now, nullable=False)
    block_description = Column(Text, default="", nullable=False)
    # columns about unblock: who? when? why?
    unblocked_by = Column(String(64), default="", nullable=False)
    unblocked_at = Column(DateTime, default=datetime.now, nullable=False)
    unblock_description = Column(Text, default="", nullable=False)

    # ToDo: rewrite its usage
    @classmethod
    def add(cls, block, group, unblocked_by, unblock_description,
            old_group_id=None, new_group_id=None, old_bin=None, new_bin=None, ancestor_id=None):
        """
        Try to add history object to history database.
        Failing that, add limbo object to main database.
        """

        group.update_updated()
        db.main.session.flush()  # ToDo: do we need this?

        # We create history objects outside of use_db, because we want to read
        # attributes on rng before we switch main engine off.
        history_object = cls.from_block(
            block, unblocked_by, unblock_description,
            old_group_id, new_group_id, old_bin, new_bin, ancestor_id
        )
        LimboClass = models.LIMBO_VERSIONS[cls.version]
        limbo_object = LimboClass.from_block(
            block, unblocked_by, unblock_description,
            old_group_id, new_group_id, old_bin, new_bin, ancestor_id
        )

        try:
            with db.history.use_master():
                db.history.session.add(history_object)
                db.history.session.commit()
        except db.DatabaseNotAvailable:
            with db.main.use_master():
                db.main.session.add(limbo_object)
                db.main.session.commit()

    @classmethod
    def from_block(cls, block, unblocked_by, unblock_description,
                    old_group_id=None, new_group_id=None, old_bin=None, new_bin=None, ancestor_id=None):
        args = {
            "group_id": block.group_id,
            "blocked_by": block.user,
            "blocked_at": block.created,
            "block_description": block.block_descr,
            "unblocked_by": unblocked_by,
            "unblocked_at": datetime.now(),
            "unblock_description": unblock_description
        }

        if old_group_id is not None:
            args["group_id"] = old_group_id

        if hasattr(block, "id"):
            args["rule_id"] = block.id
            if new_group_id is not None:
                args["new_group_id"] = new_group_id
            args["current_bin"] = old_bin
            args["new_bin"] = new_bin
            if ancestor_id is not None:
                args["ancestor_id"] = ancestor_id

        if hasattr(cls, "value_key"):
            args[cls.value_key] = getattr(block, block.value_key)
        else:
            args["rng_start"] = block.rng_start
            args["rng_end"] = block.rng_end

        return cls(**args)

    # ToDo: make it shorter
    @classmethod
    def from_limbo(cls, limbo):
        args = {
            "group_id": limbo.group_id,
            "blocked_by": limbo.blocked_by,
            "blocked_at": limbo.blocked_at,
            "block_description": limbo.block_description,
            "unblocked_by": limbo.unblocked_by,
            "unblocked_at": limbo.unblocked_at,
            "unblock_description": limbo.unblock_description
        }

        if hasattr(limbo, "rule_id"):
            args["id"] = limbo.rule_id

        if hasattr(cls, "value_key"):
            args[cls.value_key] = getattr(limbo, limbo.value_key)
        else:
            args["rng_start"] = limbo.rng_start
            args["rng_end"] = limbo.rng_end

        return cls(**args)


# Unquote `unblock_description` automatically on all HistoryProto derived classes
@event.listens_for(HistoryPrototype, "before_insert", propagate=True)
def unquote_unblock_description(mapper, connection, target):
    if target.unblock_description:
        target.unblock_description = target.unblock_description. \
            replace("\"", "").replace("'", "").strip()
