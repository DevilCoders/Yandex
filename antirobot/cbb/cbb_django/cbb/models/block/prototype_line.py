import antirobot.cbb.cbb_django.cbb.library.db as db
import antirobot.cbb.cbb_django.cbb.models as models
from antirobot.cbb.cbb_django.cbb.library.errors import (BadGroupTypeError, InvalidTxtRangeError)
from django.utils.translation import ugettext_lazy as _
from sqlalchemy import (BigInteger, Column)
from antirobot.cbb.cbb_django.cbb.library.common import change_bin
import logging

from . import BlockPrototype

logger = logging.getLogger("cbb.views")


class BlockLinePrototype(BlockPrototype):
    id = Column(BigInteger, primary_key=True,
                autoincrement=True)

    @classmethod
    def find_one(cls, txt_id):
        return cls.query.filter_by(id=txt_id).one()

    @classmethod
    def q_find(cls, txt=None, groups=None, exact=True):
        """
        Builds a query that finds text ranges.
        """
        query = cls.query
        if groups:
            query = query.filter(cls.group_id.in_(groups))

        if txt:
            if not exact:
                query = query.filter(getattr(cls, cls.value_key).like("%" + txt + "%"))
            else:
                query = query.filter_by(**{cls.value_key: txt})

        return query

    @classmethod
    def add(cls, txt, group, block_descr="", user="",
            scold_on_repeat=True, expire=None):
        """
        Adds block and history objects after checking
        """
        if not isinstance(txt, (list, tuple)):
            txt = (txt,)

        expire = group.increase_expire(expire) or None
        if group.default_type not in ("3", "4"):
            raise BadGroupTypeError(_("Wrong group type"))

        if not group.is_internal:
            internal_groups = models.Group.get_internal_ids(group_types=(group.default_type,))
            if internal_groups:
                for txt_entry in txt:
                    if cls.q_find(txt, internal_groups).first():
                        raise InvalidTxtRangeError(_("Exists in internal group"))

        for txt_entry in txt:
            # If already exists - change expire
            same_block = cls.q_find(txt_entry, [group.id]).first()

            if same_block:
                if block_descr is not None and len(block_descr) > 0:
                    same_block.block_descr = block_descr
                same_block.expire = expire
                new_block = same_block
            else:
                cls_kwargs = {
                    cls.value_key: txt_entry,
                    "group_id": group.id,
                    "user": user,
                    "block_descr": block_descr,
                    "expire": expire,
                }

                new_block = cls(**cls_kwargs)

            db.main.session.add(new_block)

        if len(txt) > 0:
            group.update_updated()

        return new_block

    @classmethod
    def copy(cls, user, group, new_group, block, description, new_expire, old_bin, exp_bin):
        if group.default_type not in ("3", "4") and new_group.default_type not in ("3", "4"):
            raise BadGroupTypeError(_("Wrong group type"))

        if cls.version == 'txt':
            rng_txt = block.rng_txt
        else:
            rng_txt = block.rng_re

        rng_txt = change_bin(rng_txt, exp_bin)

        if new_expire is None:
            new_expire = block.expire

        if description is None or len(description) == 0:
            description = block.block_descr

        new_block = cls.add(rng_txt, new_group, description, user, expire=new_expire)

        if exp_bin == '-2':
            exp_bin = old_bin
        elif exp_bin == '-1':
            exp_bin = None

        models.HISTORY_VERSIONS[cls.version].add(
            block=new_block,
            group=new_group,
            unblocked_by=user,
            unblock_description="None",
            old_group_id=group.id,
            new_group_id=new_group.id,
            old_bin=old_bin,
            new_bin=exp_bin,
            ancestor_id=block.id
        )

    @classmethod
    def move(cls, user, group, new_group, block, description, new_expire, old_bin, exp_bin):
        if group.default_type not in ("3", "4") and new_group.default_type not in ("3", "4"):
            raise BadGroupTypeError(_("Wrong group type"))

        if cls.version == 'txt':
            rng_txt = block.rng_txt
        else:
            rng_txt = block.rng_re

        rng_txt = change_bin(rng_txt, exp_bin)

        if exp_bin == '-2':
            exp_bin = old_bin
        elif exp_bin == '-1':
            exp_bin = None

        same_block = cls.q_find(rng_txt, [new_group.id]).first()
        if same_block is not None:
            if old_bin != exp_bin and group.id == new_group.id or group.id != new_group.id:
                cls.delete(group, txt_id=block.id, operation_descr="Already existed")
            block = same_block

        block.group_id = new_group.id

        if old_bin != exp_bin:
            if cls.version == 'txt':
                block.rng_txt = rng_txt
            else:
                block.rng_re = rng_txt

        if new_expire is not None:
            block.expire = new_expire

        models.HISTORY_VERSIONS[cls.version].add(
            block=block,
            group=group,
            unblocked_by=user,
            unblock_description=description,
            old_group_id=group.id,
            new_group_id=new_group.id,
            old_bin=old_bin,
            new_bin=exp_bin,
        )

        if description is not None and len(description) > 0:
            block.block_descr = description

        group.update_updated()
        new_group.update_updated()

    @classmethod
    def delete(cls, group, txt="", txt_id=None, user="", operation_descr=""):
        if txt_id is not None:
            blocks_to_delete = cls.query.filter_by(id=txt_id).all()
        elif txt != "":
            blocks_to_delete = cls.q_find(txt, [group.id]).all()
        else:
            raise InvalidTxtRangeError(_("No entity given to delete"))

        for block in blocks_to_delete:
            models.HISTORY_VERSIONS[cls.version].add(
                block=block,
                group=group,
                unblocked_by=user,
                unblock_description=operation_descr
            )
            db.main.session.delete(block)
