import re
import schematics.exceptions

from schematics.types import StringType, ListType, IntType, ModelType

from yc_common.models import Model


# Route Table related models
class CommunityAttributeType(StringType):
    """
        List of Community attributes
        This list indicates the attributes with which routes are tagged while
        publishing. The attributes will be represented as bgp community in
        the path attribute. Each attribute is indicated as string
            1. String with two integer separated by  ':'. E.g. "64512:123"
            2. Well-known community as string.  Possible values are
                "no-export"
                "accept-own"
                "no-advertise"
                "no-export-subconfed"
                "no-reoriginate"
    """

    class WellKnownCommunity:
        NO_EXPORT = "no-export"
        ACCEPT_OWN = "accept-own"
        NO_ADVERTISE = "no-advertise"
        NO_EXPORT_SUBCONFED = "no-export-subconfed"
        NO_REORIGINATE = "no-reoriginate"

        ALL = [NO_EXPORT, ACCEPT_OWN, NO_ADVERTISE, NO_EXPORT_SUBCONFED, NO_REORIGINATE]

    COMMUNITY_RE = re.compile(r"^(\d{1,5}):(\d{1,10})$")

    def validate_community_attribute(self, value, context=None):
        if value not in self.WellKnownCommunity.ALL:
            match = self.COMMUNITY_RE.match(value)
            if match is None:
                raise schematics.exceptions.ValidationError("Invalid community attribute format: {}.", value)


class CommunityAttributes(Model):
    community_attribute = ListType(CommunityAttributeType)


class RouteType(Model):
    class RouteNextHopType:
        SERVICE_INSTANCE = "service-instance"
        IP_ADDRESS = "ip-address"

        ALL = [SERVICE_INSTANCE, IP_ADDRESS]

    prefix = StringType()
    next_hop = StringType()
    next_hop_type = StringType(choices=RouteNextHopType.ALL, default=RouteNextHopType.IP_ADDRESS)
    community_attributes = ModelType(CommunityAttributes)


class RouteTableType(Model):
    route = ListType(ModelType(RouteType))


# Routing Policy related models
class CommunityListType(Model):
    community = ListType(CommunityAttributeType)


class ActionCommunityType(Model):
    add = ModelType(CommunityListType)
    remove = ModelType(CommunityListType)
    set = ModelType(CommunityListType)


class ActionUpdateType(Model):
    community = ModelType(ActionCommunityType)
    local_pref = IntType()
    med = IntType()


class TermActionListType(Model):
    class ActionType:
        REJECT = "reject"
        ACCEPT = "accept"
        NEXT = "next"

        ALL = [REJECT, ACCEPT, NEXT]

    update = ModelType(ActionUpdateType, default={})
    action = StringType(choices=ActionType.ALL)


class PrefixMatchType(Model):
    class PrefixType:
        EXACT = "exact"
        LONGER = "longer"
        ORLONGER = "orlonger"

        ALL = [EXACT, LONGER, ORLONGER]

    prefix = StringType()
    prefix_type = StringType(choices=PrefixType.ALL, default=PrefixType.EXACT)


class TermMatchConditionType(Model):
    class PathSourceType:
        BGP = "bgp"
        XMPP = "xmpp"
        STATIC = "static"
        SERVICE_CHAIN = "service-chain"
        AGGREGATE = "aggregate"

        ALL = [BGP, XMPP, STATIC, SERVICE_CHAIN, AGGREGATE]

    protocol = ListType(StringType(choices=PathSourceType.ALL), default=[])
    community = StringType(serialize_when_none=False)
    prefix = ListType(ModelType(PrefixMatchType), serialize_when_none=False)


class PolicyTermType(Model):
    term_match_condition = ModelType(TermMatchConditionType)
    term_action_list = ModelType(TermActionListType)


class PolicyStatementType(Model):
    term = ListType(ModelType(PolicyTermType))


class SequenceType(Model):
    sequence = StringType()
