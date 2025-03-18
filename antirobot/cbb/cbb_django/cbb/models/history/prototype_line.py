import sqlalchemy

from antirobot.cbb.cbb_django.cbb import models
from antirobot.cbb.cbb_django.cbb.models import group

from . import HistoryPrototype


class HistoryLinePrototype(HistoryPrototype):
    rule_id = sqlalchemy.Column(sqlalchemy.BigInteger, index=True)
    # columns about moving blocks: destination? old bin? new bin?
    new_group_id = sqlalchemy.Column(sqlalchemy.Integer, nullable=True)
    current_bin = sqlalchemy.Column(sqlalchemy.Integer, nullable=True)
    new_bin = sqlalchemy.Column(sqlalchemy.Integer, nullable=True)
    # column about block's ancestor (if block was created after coping another)
    ancestor_id = sqlalchemy.Column(sqlalchemy.BigInteger, index=True)

    # ToDo: rewrite this
    @classmethod
    def find(cls, txt=None, group_id=None, rule_id=None):
        query = cls.query

        if txt is not None:
            query = query.filter(getattr(cls, cls.value_key).like("%" + txt + "%"))

        if group_id or rule_id is not None:
            query_params = {}

            if group_id:
                query_params["group_id"] = group_id

            if rule_id is not None:
                query_params["rule_id"] = rule_id

            query = query.filter_by(**query_params)

        if group_id is None and txt is None and rule_id is None:
            query = query.select_from(cls)

        return query.order_by(sqlalchemy.desc(cls.unblocked_at))

    @classmethod
    def find_one(cls, rule_id=None):
        return cls.query.filter_by(rule_id=rule_id).first()

    @classmethod
    def prepare_output(cls, txt=None, group_id=None):
        """
        Find active and history blocks for particular txt
        and prepare results for interface.
        """

        # Get active blocks
        groups = [group_id] if group_id else []
        active_blocks = models.BLOCK_VERSIONS[cls.version].q_find(txt, groups, exact=False).all()
        # Get history blocks
        history_blocks = cls.find(txt, group_id).all()

        try:
            rule_id = int(txt)
        except ValueError:
            rule_id = None

        if rule_id is not None:
            try:
                active_blocks.append(models.BLOCK_VERSIONS[cls.version].find_one(rule_id))
            except sqlalchemy.orm.exc.NoResultFound:
                pass

            history_blocks += cls.find(group_id=group_id, rule_id=rule_id).all()

        # Get uniq group IDs
        uniq_group_ids = set([b.group_id for b in active_blocks] + [b.group_id for b in history_blocks])

        result = []
        for group_id in sorted(uniq_group_ids):
            # get every uniq txt
            gr = group.Group.query.get(group_id)
            uniq_blocks = set()
            active_blocks_txts = [getattr(e, e.value_key) for e in active_blocks if e.group_id == group_id]
            history_blocks_txts = [getattr(e, e.value_key) for e in history_blocks if e.group_id == group_id]
            uniq_blocks = set(active_blocks_txts + history_blocks_txts)
            for b in sorted(uniq_blocks):
                output = {"group": gr, "block": b}
                if b in active_blocks_txts:
                    output["active"] = [e for e in active_blocks if e.group_id == group_id and getattr(e, e.value_key) == b][0]
                if b in history_blocks_txts:
                    # ToDo: key=lala.unblocked_at, reverse=True
                    output["history"] = sorted(
                        [
                            e for e in history_blocks
                            if e.group_id == group_id and getattr(e, e.value_key) == b
                        ],
                        key=lambda x: x.unblocked_at,
                        reverse=True,
                    )
                result.append(output)
        return result

    @classmethod
    def get_txt_block_history(cls, rule_id):
        history_blocks = cls.find(rule_id=rule_id).all()
        if history_blocks is not None and len(history_blocks) > 0:
            ancestor_id = history_blocks[-1].ancestor_id
            timestamp = history_blocks[-1].unblocked_at
            if ancestor_id is not None:
                prev_history = cls.get_txt_block_history(ancestor_id)
                history_blocks.extend([event for event in prev_history if event.unblocked_at < timestamp])
        return history_blocks
