from serp import RawSerpDataStorage  # noqa
from serp import ParsedSerpDataStorage  # noqa
from serp import SerpAttrs


# pool-level context
class PoolParseContext(object):
    def __init__(self, serp_attrs, raw_serp_storage=None, parsed_serp_storage=None, threads=None,
                 allow_no_position=False, allow_broken_components=False, remove_raw_serpsets=False):
        """
        :type serp_attrs: SerpAttrs
        :type raw_serp_storage: RawSerpDataStorage | None
        :type parsed_serp_storage: ParsedSerpDataStorage | None
        :type allow_no_position: bool
        :type remove_raw_serpsets: bool
        """
        if not isinstance(serp_attrs, SerpAttrs):
            raise Exception("Wrong serp_attrs object type in PoolParseContext")
        self.serp_attrs = serp_attrs
        self.raw_serp_storage = raw_serp_storage
        self.parsed_serp_storage = parsed_serp_storage
        # TODO: move threads, allow_no_position to ParseSettings structure
        self.threads = threads
        # allow to fetch serps without 'conponentInfo.rank' (position) field
        self.allow_no_position = allow_no_position
        # allow broken SERP components (after QESUP-4995, MSTAND-1177)
        self.allow_broken_components = allow_broken_components
        self.remove_raw_serpsets = remove_raw_serpsets


# serpset-level context
class SerpsetParseContext(object):
    def __init__(self, pool_parse_ctx, serpset_id):
        """
        :type pool_parse_ctx: PoolParseContext
        :type serpset_id: str
        """
        self.pool_parse_ctx = pool_parse_ctx
        self.serpset_id = serpset_id
