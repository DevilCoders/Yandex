import antirobot.cbb.cbb_django.cbb.models as models
from antirobot.cbb.cbb_django.cbb.library.common import get_ip_from_string
from antirobot.cbb.cbb_django.cbb.library.errors import InvalidRangeError
from ipaddr import Bytes, IPAddress, v6_int_to_packed
from sqlalchemy import (BigInteger, Boolean, Column, LargeBinary, desc, event, sql)
from sqlalchemy.ext.declarative import declared_attr

from . import HistoryPrototype


# TODO: add __repr__ to this class
class HistoryIPPrototype(HistoryPrototype):
    """
    Base prototype for ipv4 and ipv6 history and limbos.
    IP columns are parametrized.
    """
    version = None

    # This column indicates whether range is longer than 255 addresses.
    xtra_large = Column(Boolean, default=False, server_default=sql.literal_column("false"), nullable=False)

    # ToDo: rewrite this
    @classmethod
    def _get_ip_column(cls):
        ver = cls.version
        assert ver in {4, 6}
        if ver == 4:
            return Column(BigInteger, nullable=False, default=0)
        return Column(LargeBinary(16), nullable=False, default=0)

    rng_start = declared_attr(lambda cls: cls._get_ip_column())
    rng_end = declared_attr(lambda cls: cls._get_ip_column())

    # ToDo: rewrite this
    @classmethod
    def get_ipaddr(cls, raw):
        ver = cls.version
        assert ver in {4, 6}
        if ver == 4:
            return IPAddress(raw, 4)
        return IPAddress(Bytes(raw), 6)

    # ToDo: rewrite this
    def get_rng_start(self):
        return self.get_ipaddr(self.rng_start)

    # ToDo: rewrite this
    def get_rng_end(self):
        return self.get_ipaddr(self.rng_end)

    def get_full_range(self):
        if self.rng_start == self.rng_end:
            return str(self.get_rng_start())
        else:
            rng_start = self.get_rng_start()
            rng_end = self.get_rng_end()
            return str(rng_start) + " - " + str(rng_end)

    # ToDo: rewrite this
    @classmethod
    def find(cls, rng_start=None, rng_end=None, group_id=None):
        query = cls.query

        rng_start = rng_start and get_ip_from_string(rng_start, cls.version, result="int")
        rng_end = rng_end and get_ip_from_string(rng_end, cls.version, result="int")

        query = cls.apply_filter_by_range(query, rng_start, rng_end)

        if group_id:
            query = query.filter_by(group_id=group_id)

        query = query.order_by(desc(cls.unblocked_at))

        return query

    @classmethod
    def prepare_output(cls, rng_start=None, group_id=None):
        """
        Find active and history blocks for particular rng_start
        and prepare results for interface.
        """

        rng_end = rng_start

        if rng_start.endswith(".0"):
            rng_end = rng_start[:-2] + ".255"

        # Get active blocks
        if rng_start and rng_end:
            rng_start = get_ip_from_string(rng_start, cls.version)
            rng_end = get_ip_from_string(rng_end, cls.version)

        if rng_end < rng_start:
            raise InvalidRangeError(("Range\"s start is greater than end"))

        active_blocks = models.BLOCK_VERSIONS[cls.version].q_block_crossings(rng_start=rng_start, rng_end=rng_end, group_id=group_id).all()
        # Get history blocks
        history_blocks = cls.find(rng_start=rng_start, rng_end=rng_end, group_id=group_id).all()

        if rng_end:
            rng_end = get_ip_from_string(rng_end, cls.version)

        active_blocks = models.BLOCK_VERSIONS[cls.version].q_block_crossings(rng_start=rng_start, rng_end=rng_end, group_id=group_id).all()
        history_blocks = cls.find(rng_start=rng_start, rng_end=rng_end, group_id=group_id).all()

        # Get uniq group IDs
        uniq_group_ids = set([b.group_id for b in active_blocks] + [b.group_id for b in history_blocks])

        result = []
        for group_id in sorted(uniq_group_ids):
            # get rng_start + rng_end uniq pairs
            group = models.Group.query.get(group_id)
            active_block_rngs = [e.get_full_range() for e in active_blocks if e.group_id == group_id]
            history_block_rngs = [e.get_full_range() for e in history_blocks if e.group_id == group_id]
            uniq_blocks = set(active_block_rngs + history_block_rngs)
            for block in sorted(uniq_blocks):
                # prepare output for each pair
                output = {"group": group, "block": block}
                if block in active_block_rngs:
                    output["active"] = [e for e in active_blocks if e.group_id == group_id and e.get_full_range() == block][0]
                if block in history_block_rngs:
                    output["history"] = sorted([e for e in history_blocks if e.group_id == group_id and e.get_full_range() == block], key=lambda b: b.unblocked_at, reverse=True)
                result.append(output)
        return result

    # ToDo: rewrite this
    @classmethod
    def apply_filter_by_range(cls, query, rng_start, rng_end):
        if cls.version == 6:
            rng_start = v6_int_to_packed(rng_start)
            rng_end = v6_int_to_packed(rng_end)

        query = query.filter(
            ((cls.rng_start <= rng_start) & (cls.rng_end >= rng_start))
            | ((cls.rng_start > rng_start) & (cls.rng_start <= rng_end))
        )

        return query


# Set `xtra_large` flat automatically on all HistoryIpProto derived classes
@event.listens_for(HistoryIPPrototype, "before_insert", propagate=True)
def update_geom_column(mapper, connection, target):
    target.xtra_large = (int(target.get_rng_end()) - int(target.get_rng_start())) > 255
