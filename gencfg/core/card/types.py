from datetime import date, timedelta
import re
from collections import defaultdict

from gaux.aux_staff import get_possible_group_owners


class EStatuses(object):
    STATUS_OK = 0
    STATUS_FAIL = 1


class TValidatedValue(object):
    __slots__ = ['status', 'value', 'reason']

    def __init__(self, status, value, reason):
        self.status = status
        self.value = value
        self.reason = reason


class ICardType(object):
    __slots__ = []

    # check if value is correct (for int type check if value is int, for positive float check if it is positive float ...
    def validate(self, value):
        raise NotImplementedError("Found not implemented function <validate>")

    # check if value is correct for update purposes (some values, which are valid for read could be not valid for write, like fired group owners in tags)
    # here we also checking more complex constraints like having ipv4/ipv6 addrs, appropriate net cards, etc.
    def validate_for_update(self, group, value):
        return self.validate(value)

    # return list of available values, None if all possible values can not be specified as finite list
    def get_avail_values(self, db):
        return None


"""
    Auxiliarily classes, representing custom types (can be used from anywhere)
"""


class ByteSize(object):
    SUFFIXES = {
        'B': 1,

        'KBIT': 1024 / 8,
        'KB': 1024,

        'MBIT': 1024 * 1024 / 8,
        'MB': 1024 * 1024,

        'GBIT': 1024 * 1024 * 1024 / 8,
        'GB': 1024 * 1024 * 1024,

        'TBIT': 1024 * 1024 * 1024 * 1024 / 8,
        'TB': 1024 * 1024 * 1024 * 1024,
    }

    SPLIT_REGEX = re.compile('([\d.]+)\s*(.*)')

    __slots__ = ['text', 'value']

    def __init__(self, text):
        self.reinit(text)

    def reinit(self, text):
        m = re.match(ByteSize.SPLIT_REGEX, text)
        if not m:
            raise Exception("String <%s> can not be parsed as byte size" % text)

        sz, suffix = float(m.group(1)), m.group(2).upper()
        if sz < 0:
            raise Exception("Got negative ByteSize when parsing <%s>" % text)
        if suffix not in ByteSize.SUFFIXES:
            raise Exception("Unknown suffix <%s> when parsing <%s>" % (suffix, text))

        self.value = sz * ByteSize.SUFFIXES[suffix]
        self.text = text

    def __str__(self):
        return self.text

    def __eq__(self, other):
        return isinstance(other, self.__class__) and self.value == other.value

    def __ne__(self, other):
        return not (self == other)

    def __lt__(self, other):
        return self.value < other.value

    def __le__(self, other):
        return self.value <= other.value

    def __gt__(self, other):
        return self.value > other.value

    def __ge__(self, other):
        return self.value >= other.value

    def __nonzero__(self):
        return self.value > 0

    def megabytes(self):
        return self.value / (2 ** 20)

    def gigabytes(self):
        return self.value / (2 ** 30)

    def megabits(self):
        return self.value * 8 / (2 ** 30)


class Function(object):
    def __init__(self, text, function):
        self.text = text
        self.function = function

    def __call__(self, *args, **kwargs):
        return self.function(*args, **kwargs)

    def __str__(self):
        return self.text

    def __eq__(self, other):
        return isinstance(other, self.__class__) and self.text == other.text

    def __getstate__(self):
        return self.text

    def __setstate__(self, state):
        self.text = state
        self.function = eval(state)


class Date(object):
    MONTHS = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec']

    def __init__(self):
        pass

    @classmethod
    def create_from_text(cls, text):
        self = cls()
        text = text.replace(',', '')
        month, day, year = text.split(' ')
        month = Date.MONTHS.index(month) + 1
        day = int(day)
        year = int(year)
        self.date = date(year, month, day)
        return self

    @classmethod
    def create_from_duration(cls, duration):
        self = cls()
        self.date = date.today() + timedelta(duration)
        return self

    @classmethod
    def create_from_timestamp(cls, timestamp):
        self = cls()
        self.date = date.fromtimestamp(timestamp)
        return self

    def __str__(self):
        res = '%s %s, %s' % (Date.MONTHS[self.date.month - 1], self.date.day, self.date.year)
        return res

    def __eq__(self, other):
        return isinstance(other, self.__class__) and self.date == other.date

    def __ne__(self, other):
        return not (self == other)

    def __getstate__(self):
        return self.date.year, self.date.month, self.date.day

    def __setstate__(self, state):
        self.date = date(*state)


"""
    Basic types
"""


class StringType(ICardType):
    def validate(self, value):
        if isinstance(value, str):
            return TValidatedValue(EStatuses.STATUS_OK, value, None)
        elif isinstance(value, unicode):
            return TValidatedValue(EStatuses.STATUS_OK, value.encode('utf-8'), None)
        else:
            return TValidatedValue(EStatuses.STATUS_FAIL, None,
                                   "Inappropriate type <%s> (expected <str>)" % type(value))


class BoolType(ICardType):
    def validate(self, value):
        if isinstance(value, bool):
            return TValidatedValue(EStatuses.STATUS_OK, value, None)
        elif isinstance(value, int):
            try:
                value = int(value)
                if value not in [0, 1]:
                    return TValidatedValue(EStatuses.STATUS_FAIL, None, "Can not convert int <%s> to bool" % value)
                return TValidatedValue(EStatuses.STATUS_OK, value, None)
            except ValueError:
                return TValidatedValue(EStatuses.STATUS_FAIL, None, "Can not convert <%s> to bool" % value)
        else:
            return TValidatedValue(EStatuses.STATUS_FAIL, None,
                                   "Do not know how to convert <%s> type to bool type" % type(value))


class IntType(ICardType):
    def validate(self, value):
        if isinstance(value, int):
            return TValidatedValue(EStatuses.STATUS_OK, value, None)
        elif isinstance(value, str):
            try:
                value = int(value)
                return TValidatedValue(EStatuses.STATUS_OK, value, None)
            except ValueError:
                return TValidatedValue(EStatuses.STATUS_FAIL, None, "Can not convert <%s> to int" % value)
        else:
            return TValidatedValue(EStatuses.STATUS_FAIL, None,
                                   "Do not know how to convert <%s> type to int type" % type(value))


class FloatType(ICardType):
    def validate(self, value):
        if isinstance(value, (int, float)):
            return TValidatedValue(EStatuses.STATUS_OK, float(value), None)
        elif isinstance(value, str):
            try:
                value = float(value)
                return TValidatedValue(EStatuses.STATUS_OK, value, None)
            except ValueError:
                return TValidatedValue(EStatuses.STATUS_FAIL, None, "Can not convert <%s> to float" % value)
        else:
            return TValidatedValue(EStatuses.STATUS_FAIL, None,
                                   "Do not know how to convert <%s> type to float type" % type(value))


class ByteSizeType(ICardType):
    def validate(self, value):
        if isinstance(value, ByteSize):
            return TValidatedValue(EStatuses.STATUS_OK, value, None)
        elif isinstance(value, str):
            return TValidatedValue(EStatuses.STATUS_OK, ByteSize(value), None)
        else:
            return TValidatedValue(EStatuses.STATUS_FAIL, None,
                                   "Do not know how to convert <%s> type to ByteSize type" % type(value))


class FunctionType(ICardType):
    def validate(self, value):
        if hasattr(value, '__call__'):
            return TValidatedValue(EStatuses.STATUS_OK, value, None)
        elif isinstance(value, str):
            try:
                evaled = eval(value)
            except Exception:
                return TValidatedValue(EStatuses.STATUS_FAIL, None, "Can not eval <%s>" % value)

            return TValidatedValue(EStatuses.STATUS_OK, Function(value, evaled), None)
        else:
            return TValidatedValue(EStatuses.STATUS_FAIL, None,
                                   "Do not know how to convert <%s> type to Function type" % type(value))


class DateType(ICardType):
    def validate(self, value):
        if isinstance(value, Date):
            return TValidatedValue(EStatuses.STATUS_OK, value, None)
        elif isinstance(value, (str, int)):
            try:
                return TValidatedValue(EStatuses.STATUS_OK, Date.create_from_text(value), None)
            except Exception:
                try:
                    value = int(value)
                    return TValidatedValue(EStatuses.STATUS_OK, Date.create_from_duration(value), None)
                except Exception:
                    if value == "None":
                        return TValidatedValue(EStatuses.STATUS_OK, None, None)
                    return TValidatedValue(EStatuses.STATUS_FAIL, None, "Can not convert <%s> to Date" % value)
        else:
            return TValidatedValue(EStatuses.STATUS_FAIL, None,
                                   "Do not know how to convert <%s> type to Date type" % type(value))


"""
    More complex basic types like owners, metaprojects, ...
"""


class GroupOwnerType(ICardType):
    def validate(self, value):
        if isinstance(value, str):
            return TValidatedValue(EStatuses.STATUS_OK, value, None)
        else:
            return TValidatedValue(EStatuses.STATUS_FAIL, None,
                                   "Inappropriate type <%s> for GroupOwner (expected <str>)" % type(value))

    def validate_for_update(self, group, value):
        status = self.validate(value)
        if status.status == EStatuses.STATUS_FAIL:
            return status

        if value in get_possible_group_owners():
            return TValidatedValue(EStatuses.STATUS_OK, value, None)
        else:
            return TValidatedValue(EStatuses.STATUS_FAIL, None,
                                   "Users <%s> are not found or do not belong to admin department" % value)


class UserType(ICardType):
    def validate(self, value):
        if isinstance(value, str):
            return TValidatedValue(EStatuses.STATUS_OK, value, None)
        else:
            return TValidatedValue(EStatuses.STATUS_FAIL, None,
                                   "Inappropriate type <%s> for User (expected <str>)" % type(value))

    def validate_for_update(self, group, value):
        return TValidatedValue(EStatuses.STATUS_OK, value, None)
        status = self.validate(value)
        if status.status == EStatuses.STATUS_FAIL:
            return status

        avail_users = get_possible_group_owners()

        if value in avail_users:
            return TValidatedValue(EStatuses.STATUS_OK, value, None)
        else:
            return TValidatedValue(EStatuses.STATUS_FAIL, None,
                                   "Users <%s> are not found or do not belong to admin department" % value)


class CtypeType(ICardType):
    def validate(self, value):
        if isinstance(value, str):
            return TValidatedValue(EStatuses.STATUS_OK, value, None)
        else:
            return TValidatedValue(EStatuses.STATUS_FAIL, None,
                                   "Inappropriate type <%s> for Ctype (expected <str>)" % type(value))

    def validate_for_update(self, group, value):
        mydb = group.parent.db
        if mydb.version >= "2.2.20":  # can not check older bases, because there are a lot of wrong values there
            if isinstance(value, str):
                if mydb.ctypes.has_ctype(value):
                    return TValidatedValue(EStatuses.STATUS_OK, value, None)
                else:
                    return TValidatedValue(EStatuses.STATUS_FAIL, None, "Unknown ctype <%s>" % value)
            else:
                return TValidatedValue(EStatuses.STATUS_FAIL, None,
                                       "Inappropriate type <%s> for Ctype (expected <str>)" % type(value))

    def get_avail_values(self, db):
        return map(lambda x: x.name, db.ctypes.get_ctypes())


class ItypeType(ICardType):
    def validate(self, value):
        if isinstance(value, str):
            return TValidatedValue(EStatuses.STATUS_OK, value, None)
        else:
            return TValidatedValue(EStatuses.STATUS_FAIL, None,
                                   "Inappropriate type <%s> for Itype (expected <str>)" % type(value))

    def validate_for_update(self, group, value):
        mydb = group.parent.db
        if mydb.version >= "2.2.20":  # can not check older bases, because there are a lot of wrong values there
            if isinstance(value, str):
                if mydb.itypes.has_itype(value):
                    return TValidatedValue(EStatuses.STATUS_OK, value, None)
                else:
                    return TValidatedValue(EStatuses.STATUS_FAIL, None, "Unknown itype <%s>" % value)
            else:
                return TValidatedValue(EStatuses.STATUS_FAIL, None,
                                       "Inappropriate type <%s> for Itype (expected <str>)" % type(value))

    def get_avail_values(self, db):
        return map(lambda x: x.name, db.itypes.get_itypes())


class PrjType(ICardType):
    def validate(self, value):
        REG = "^[a-z][a-z0-9-]+$"
        if isinstance(value, str):
            if re.match(REG, value):
                return TValidatedValue(EStatuses.STATUS_OK, value, None)
            else:
                return TValidatedValue(EStatuses.STATUS_FAIL, None,
                                       "Group prj name <%s> does not satisfy prj regexp <%s>" % (value, REG))
        else:
            return TValidatedValue(EStatuses.STATUS_FAIL, None,
                                   "Inappropriate type <%s> for Prj (expected <str>)" % type(value))


class MetaprjType(ICardType):
    def validate(self, value):
        if isinstance(value, str):
            return TValidatedValue(EStatuses.STATUS_OK, value, None)
        else:
            return TValidatedValue(EStatuses.STATUS_FAIL, None,
                                   "Inappropriate type <%s> for Metaprj (expected <str>)" % type(value))

    def validate_for_update(self, group, value):
        mydb = group.parent.db
        if mydb.version >= "2.2.20":  # can not check older bases, because there are a lot of wrong values there
            if isinstance(value, str):
                if value in mydb.constants.METAPRJS.keys():
                    return TValidatedValue(EStatuses.STATUS_OK, value, None)
                else:
                    return TValidatedValue(EStatuses.STATUS_FAIL, None, "Unknown metaprj <%s>" % value)
            else:
                return TValidatedValue(EStatuses.STATUS_FAIL, None,
                                       "Inappropriate type <%s> for Metaprj (expected <str>)" % type(value))
        else:
            return TValidatedValue(EStatuses.STATUS_OK, value, None)

    def get_avail_values(self, db):
        return db.constants.METAPRJS.keys()


class InstancePortFuncType(ICardType):
    def validate(self, value):
        if isinstance(value, str):
            if re.match("(old\d+|new\d+|auto|default)$", value):
                return TValidatedValue(EStatuses.STATUS_OK, value, None)
            else:
                return TValidatedValue(EStatuses.STATUS_FAIL, None,
                                       "Port func <%s> does not satisfy portFunc regular expression <(old\d+|new\d+|auto|default)$>" % value)
        else:
            return TValidatedValue(EStatuses.STATUS_FAIL, None,
                                   "Inappropriate type <%s> for InstancePortFunc (expected <str>)" % type(value))


class TierType(ICardType):
    def validate(self, value):
        if isinstance(value, str):
            if re.match("^[a-zA-Z0-9]*$", value) is not None:
                return TValidatedValue(EStatuses.STATUS_OK, value, None)
            else:
                return TValidatedValue(EStatuses.STATUS_FAIL, None,
                                       "Tier name <%s> mismatch: can contain only <a-zAZ0-9> symbols" % value)
        else:
            return TValidatedValue(EStatuses.STATUS_FAIL, None,
                                   "Inappropriate type <%s> for Tier (expected <str>)" % type(value))

    def validate_for_update(self, group, value):
        if isinstance(value, str):
            if value in group.parent.db.tiers.get_tier_names() + [""]:
                return TValidatedValue(EStatuses.STATUS_OK, value, None)
            else:
                return TValidatedValue(EStatuses.STATUS_FAIL, None, "Tier <%s> does not exists" % value)
        else:
            return TValidatedValue(EStatuses.STATUS_FAIL, None,
                                   "Inappropriate type <%s> for Tier (expected <str>)" % type(value))

    def get_avail_values(self, db):
        return list(db.tiers.get_tier_names())


"""
    Some decorators
"""


class HbfMacrosType(ICardType):
    """Hbf macros"""

    def validate(self, value):
        return TValidatedValue(EStatuses.STATUS_OK, value, None)

    def validate_for_update(self, group, value):
        if isinstance(value, str):
            return TValidatedValue(EStatuses.STATUS_OK, value, None)
        else:
            return TValidatedValue(EStatuses.STATUS_FAIL, None,
                                   "Inappropriate type <%s> for HbfMacros (expected <str>)" % type(value))


class PositiveType(ICardType):
    __slots__ = ['base']

    def __init__(self, base):
        self.base = base

    def validate(self, value):
        ret = self.base.validate(value)

        if ret.status == EStatuses.STATUS_FAIL:
            return ret
        elif ret.value <= 0:
            ret.status = EStatuses.STATUS_FAIL
            ret.reason = "Value <%s> less or equal to zero" % ret.value
            return ret
        else:
            return ret

    def get_avail_values(self, db):
        return self.base.get_avail_values(db)


class NonNegativeType(ICardType):
    __slots__ = ['base']

    def __init__(self, base):
        self.base = base

    def validate(self, value):
        ret = self.base.validate(value)

        if ret.status == EStatuses.STATUS_FAIL:
            return ret
        elif ret.value < 0:
            ret.status = EStatuses.STATUS_FAIL
            ret.reason = "Value <%s> less than zero" % ret.value
            return ret
        else:
            return ret

    def get_avail_values(self, db):
        return self.base.get_avail_values(db)


class NoneOrType(ICardType):
    __slots__ = ['base']

    def __init__(self, base):
        self.base = base

    def validate(self, value):
        if value is None:
            return TValidatedValue(EStatuses.STATUS_OK, value, None)
        else:
            return self.base.validate(value)

    def validate_for_update(self, group, value):
        if value is None:
            return TValidatedValue(EStatuses.STATUS_OK, value, None)
        else:
            return self.base.validate_for_update(group, value)

    def get_avail_values(self, db):
        return self.base.get_avail_values(db)


class ListOfType(ICardType):
    __slots__ = ['base']

    def __init__(self, base):
        self.base = base

    def validate(self, value):
        if isinstance(value, list):
            retlist = map(lambda x: self.base.validate(x), value)
            failed_retlist = filter(lambda x: x.status == EStatuses.STATUS_FAIL, retlist)

            if len(failed_retlist) > 0:
                return TValidatedValue(EStatuses.STATUS_FAIL, None, failed_retlist[0].reason)
            else:
                return TValidatedValue(EStatuses.STATUS_OK, map(lambda x: x.value, retlist), None)
        else:
            return TValidatedValue(EStatuses.STATUS_FAIL, None,
                                   "Do not know how to convert value of type <%s> to list type" % type(value))

    # FIXME: get rid off code dublication
    def validate_for_update(self, group, value):
        if isinstance(value, list):
            retlist = map(lambda x: self.base.validate_for_update(group, x), value)
            failed_retlist = filter(lambda x: x.status == EStatuses.STATUS_FAIL, retlist)

            if len(failed_retlist) > 0:
                return TValidatedValue(EStatuses.STATUS_FAIL, None, failed_retlist[0].reason)
            else:
                return TValidatedValue(EStatuses.STATUS_OK, map(lambda x: x.value, retlist), None)
        else:
            return TValidatedValue(EStatuses.STATUS_FAIL, None,
                                   "Do not know how to convert value of type <%s> to list type" % type(value))

    def get_avail_values(self, db):
        return self.base.get_avail_values(db)


"""
    Specific checker from card reqs:
        reqs.hosts.max_per_switch - check number of hosts per switch
        reqs.hosts.have_ipv6_addr - check if group has ipv6 addr
        reqs.hosts.have_ipv4_addr - check if group has ipv4 addr
        reqs.hosts.netcard_regexp - check if group has appropriate netcard
"""


class ReqsHostsMaxPerSwitch(ICardType):
    def __init__(self):
        self.base = NonNegativeType(IntType())

    def validate(self, value):
        return self.base.validate(value)

    def validate_for_update(self, group, value):
        ret = self.base.validate_for_update(group, value)
        if ret.status == EStatuses.STATUS_FAIL:
            return ret

        if ret.value == 0:  # 0 means no limit for hosts per switch
            return ret

        by_switch_counts = defaultdict(int)
        for host in group.getHosts():
            by_switch_counts[host.switch] += 1
        failed_switches = filter(lambda (switch, N): N > ret.value, by_switch_counts.iteritems())
        failed_switches = map(lambda (switch, N): switch, failed_switches)

        if len(failed_switches) > 0:
            ret.status = EStatuses.STATUS_FAIL
            ret.reason = "More than %d host in the following switches: %s" % (ret.value, ",".join(failed_switches))

        return ret


class IReqsHostsFilter(ICardType):
    def __init__(self, BaseType, check_needed_func, filter_func, error_msg):
        if BaseType is None:
            self.base = None
        else:
            self.base = BaseType()

        self.check_needed_func = check_needed_func
        self.filter_func = filter_func
        self.error_msg = error_msg

    def validate(self, value):
        if self.base is not None:
            return self.base.validate(value)
        return TValidatedValue(EStatuses.STATUS_OK, value, None)

    def validate_for_update(self, group, value):
        if self.base is not None:
            ret = self.base.validate_for_update(group, value)
        else:
            ret = TValidatedValue(EStatuses.STATUS_OK, value, None)

        if ret.status == EStatuses.STATUS_FAIL:
            return ret

        if self.check_needed_func(ret.value):
            bad_hosts = filter(lambda x: self.filter_func(ret.value, x), group.getHosts())
            if len(bad_hosts) > 0:
                ret.status = EStatuses.STATUS_FAIL
                ret.reason = "%s: %s" % (self.error_msg, ",".join(map(lambda x: x.name, bad_hosts)))

        return ret


# this functions are needed for correct pickle
def lambda1_ReqsHostsHaveIpv6Addr(value):
    return value is True


def lambda2_ReqsHostsHaveIpv6Addr(value, host):
    del value
    return host.ipv6addr == 'unknown'


class ReqsHostsHaveIpv6Addr(IReqsHostsFilter):
    def __init__(self):
        IReqsHostsFilter.__init__(
            self,
            None,
            lambda1_ReqsHostsHaveIpv6Addr,
            lambda2_ReqsHostsHaveIpv6Addr,
            "Have hosts without ipv6 addr",
        )


# this lambda functions are needed for correct pickle
def lambda1_ReqsHostsHaveIpv4Addr(value):
    return value is True


def lambda2_ReqsHostsHaveIpv4Addr(value, host):
    del value
    return host.ipv4addr == 'unknown'


class ReqsHostsHaveIpv4Addr(IReqsHostsFilter):
    def __init__(self):
        IReqsHostsFilter.__init__(
            self,
            None,
            lambda1_ReqsHostsHaveIpv4Addr,
            lambda2_ReqsHostsHaveIpv4Addr,
            "Have hosts without ipv4 addr",
        )


# this lambda functions are needed for correct pickle
def lambda1_ReqsHostsNetcardRegexp(value):
    del value
    return True


def lambda2_ReqsHostsNetcardRegexp(value, host):
    return not re.match(value, host.netcard)


class ReqsHostsNetcardRegexp(IReqsHostsFilter):
    def __init__(self):
        IReqsHostsFilter.__init__(
            self,
            None,
            lambda1_ReqsHostsNetcardRegexp,
            lambda2_ReqsHostsNetcardRegexp,
            "Have hosts with incorrect netcard",
        )


# this lambda functions are needed for correct pickle
def lambda1_ReqsHostsNdisks(value):
    del value
    return True


def lambda2_ReqsHostsNdisks(value, host):
    return host.n_disks < value


class ReqsHostsNdisks(IReqsHostsFilter):
    def __init__(self):
        IReqsHostsFilter.__init__(
            self,
            IntType,
            lambda1_ReqsHostsNdisks,
            lambda2_ReqsHostsNdisks,
            "Have host with not enough number of disks",
        )
