from antirobot.cbb.cbb_django.cbb.library.common import get_ip_from_string
from ipaddr import Bytes, IPAddress, summarize_address_range
from sqlalchemy import BigInteger, Boolean, Column, event
from sqlalchemy.dialects.postgresql import ENUM as Enum
from sqlalchemy.ext.declarative import declared_attr
from sqlalchemy.types import LargeBinary

from . import BlockPrototype

BlockTypeEnum = Enum("cidr", "range", "single_ip", "unknown", name="block_type_enum")


# TODO: add __repr__ to this class
class BlockIPPrototype(BlockPrototype):
    """
    Base prototype for for ipv4 and ipv6 history and limbos.
    IP columns are parametrized.
    """
    version = None

    is_exception = Column(Boolean, default=False, nullable=False)
    block_type = Column(BlockTypeEnum, default="range", server_default="range", nullable=False, index=True)

    @classmethod
    def _get_range_type(cls):
        if cls.version == 4:
            return BigInteger
        return LargeBinary(16)

    @declared_attr
    def rng_start(cls):
        return Column(cls._get_range_type(), nullable=False, default=0)

    @declared_attr
    def rng_end(cls):
        return Column(cls._get_range_type(), nullable=False, default=0)

    def __repr__(self):
        return "<{} group_id: {}, rng_start: {}, rng_end: {}>".format(
            self.__class__.__name__,
            self.group_id,
            str(self.get_rng_start()),
            str(self.get_rng_end())
        )

    def __eq__(self, other):
        raise RuntimeError("Comparing ranges!")

    def get_net(self):
        net = summarize_address_range(self.get_rng_start(),
                                             self.get_rng_end())
        if len(net) != 1:
            # logger.error("Not a net: %s" % self)
            #            raise Exception("Not network")
            # TODO: в сетях нашлись не-сети =\
            return None
        return net[0]

    @classmethod
    def get_ipaddr(cls, raw):
        if cls.version == 4:
            return IPAddress(raw, 4)
        return IPAddress(Bytes(raw), 6)

    def get_rng_start(self):
        return self.get_ipaddr(self.rng_start)

    def get_rng_end(self):
        return self.get_ipaddr(self.rng_end)

    def set_rng_start(self, rng_start):
        if self.version == 6:
            rng_start = IPAddress(rng_start, 6).packed
        self.rng_start = rng_start

    def set_rng_end(self, rng_end):
        if self.version == 6:
            rng_end = IPAddress(rng_end, 6).packed
        self.rng_end = rng_end

    def get_full_range(self):
        if self.rng_start == self.rng_end:
            return str(self.get_rng_start())
        else:
            rng_start = self.get_rng_start()
            rng_end = self.get_rng_end()
            return str(rng_start) + " - " + str(rng_end)

    def get_created_ts(self):
        if self.created is not None:
            return self.created.strftime("%s")
        else:
            return "0"

    def get_expire_ts(self):
        if self.expire is not None:
            return self.expire.strftime("%s")
        else:
            return "0"

    def get_cidr_csv(self, with_descr):
        net = self.get_net()
        if net is None:
            return net
        result = [str(net.network), str(net.prefixlen), str(self.group_id)]
        result = "; ".join(result)
        if with_descr:
            result += "; " + self.block_descr.encode("unicode_escape").decode("utf-8")
        return result

    def get_csv(self, with_expire=False, with_except=False):
        fields = ["range_src", "range_dst", "flag"]
        if with_except:
            fields.append("except")
        if with_expire:
            fields.append("expire")
        return self.get_csv_with_format(fields)

    def get_csv_with_format(self, fields):
        data = {
            "range_src": str(self.get_rng_start()),
            "range_dst": str(self.get_rng_end()),
            "flag": str(self.group_id),
            "expire": self.get_expire_ts(),
            "cdate": self.get_created_ts(),
            "except": str(int(self.is_exception)),
            "description": self.block_descr,
        }
        result = [data[str(f)] for f in fields]
        return "; ".join(result)

    @classmethod
    def find_one(cls, group, block_start, block_end):
        block_start = get_ip_from_string(block_start, cls.version)
        block_end = get_ip_from_string(block_end, cls.version)

        return cls.query.filter_by(rng_start=block_start, rng_end=block_end, group_id=group.id).one()

    @classmethod
    def q_block_crossings(cls, rng_start=None, rng_end=None, group_id=None):
        query_single = cls.query.filter_by(block_type="single_ip", is_exception=False)
        query_range = cls.query.filter_by(block_type="range", is_exception=False)

        query_single = query_single.filter((cls.rng_start >= rng_start) & (cls.rng_start <= rng_end))
        query_range = query_range.filter((cls.rng_end >= rng_start) & (cls.rng_start <= rng_end))

        if group_id:
            query_single = query_single.filter_by(group_id=group_id)
            query_range = query_range.filter_by(group_id=group_id)
        return query_single.union(query_range)


# ToDo: all same event to prototype
# SQLAlchemy magicks to set `block_type` flat automatically.
# (on all BlockIPPrototype derived classes)
@event.listens_for(BlockIPPrototype, "before_insert", propagate=True)
def update_block_type_column(mapper, connection, target):
    target.block_type = "single_ip" if target.rng_start == target.rng_end else "range"


@event.listens_for(BlockPrototype, "before_insert", propagate=True)
def unquote_block_descr_column(mapper, connection, target):
    if target.block_descr:
        target.block_descr = target.block_descr. \
            replace(""", "").replace(""", "").strip()
