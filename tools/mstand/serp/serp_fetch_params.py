import logging

from serp import SerpAttrs
import yaqutils.misc_helpers as umisc
import yaqutils.file_helpers as ufile
from mstand_utils import OfflineDefaultValues


# Here is a subset of typically used filters.
# refer to details at:
# https://wiki.yandex-team.ru/jandekspoisk/metrics/api/serp/filters/#komponentnyefiltry
class MetricsSerpSetFilter(object):
    TRUE = "true"  # all (no filtering)
    IDEAL = "ideal"
    ONLY_SEARCH_RESULT = "onlySearchResult"
    NO_ADV = "noAdd"  # note: it's really 'noAdd' in Metrics enums
    TOP_ADV = "topAdv"
    TOP_ADV_AND_SEARCH_RESULT_AND_WIZARDS_NO_RIGHT = "topAdvAndSearchResultsAndWizardsNoRigthFilter"
    SKIP_RIGHT_ALIGN = "skipRightAlign"  # with wizards
    WITH_RELATED_QUERIES = "withRelatedQueries"
    ONLY_RELATED_QUERIES = "onlyRelatedQueries"
    TOP_ADV_AND_SEARCH_RESULT = "topAdvAndSearchResult"

    ALL = {
        TRUE,
        IDEAL,
        ONLY_SEARCH_RESULT,
        NO_ADV,
        TOP_ADV,
        TOP_ADV_AND_SEARCH_RESULT_AND_WIZARDS_NO_RIGHT,
        SKIP_RIGHT_ALIGN,
        WITH_RELATED_QUERIES,
        ONLY_RELATED_QUERIES,
        TOP_ADV_AND_SEARCH_RESULT
    }


class SerpFetchParams(object):
    def __init__(self, aspect=None, serp_set_filter=None, pre_filter=None, serp_attrs=None,
                 skip_broken=False, retry_timeout=30, retry_count=10,
                 metrics_server=None, mc_auth_token=None, fetch_threads=OfflineDefaultValues.FETCH_THREADS,
                 convert_threads=OfflineDefaultValues.CONVERT_THREADS,
                 use_external_convertor=True, serpset_id_filter=None):
        """
        :type aspect: str | None
        :type serp_set_filter: str | None
        :type serp_attrs: SerpAttrs | None
        :type skip_broken: bool
        :type retry_timeout: float
        :type retry_count: int
        :type metrics_server: str | None
        """
        self.aspect = aspect if not umisc.is_empty_value(aspect) else None

        # TODO: support multiple serp-set-filter values
        self.serp_set_filter = serp_set_filter
        self.pre_filter = pre_filter

        if serp_set_filter is not None and serp_set_filter not in MetricsSerpSetFilter.ALL:
            logging.warning("serp-set-filter value '%s' is not known. Possibly, you've passed wrong value", serp_set_filter)

        self.serp_attrs = serp_attrs or SerpAttrs()
        self.skip_broken = skip_broken
        self.retry_timeout = retry_timeout
        self.retry_count = retry_count
        # use production by default
        self.metrics_server = metrics_server or "metrics-calculation.qe.yandex-team.ru"
        self.mc_auth_token = mc_auth_token
        self.fetch_threads = fetch_threads
        self.convert_threads = convert_threads
        self.use_external_convertor = use_external_convertor
        self.serpset_id_filter = serpset_id_filter

        logging.info("Serpset fetch params: aspect=%s, serp-set-filter=%s, pre-filter=%s, skip broken=%s",
                     self.aspect, self.serp_set_filter, self.pre_filter, self.skip_broken)
        logging.info("Using metrics server %s", self.metrics_server)
        if self.mc_auth_token:
            obf_token = umisc.obfuscate_token(self.mc_auth_token)
            logging.info("Using metrics auth token %s", obf_token)
        else:
            logging.info("Auth token not specified")

    def get_url_params(self):
        all_reqs = self.serp_attrs.build_requirement_set()

        req_param_tuples = [("requirement", req) for req in all_reqs]

        params = []
        if self.aspect:
            params = [("aspect", self.aspect)]
        if self.serp_set_filter:
            params.append(("serp-set-filter", self.serp_set_filter))
        if self.pre_filter:
            params.append(("pre-filter", self.pre_filter))

        # MSTAND-1317 test
        # params.append(("extended", "true"))

        params.extend(req_param_tuples)
        return params

    def get_headers(self):
        if self.mc_auth_token:
            headers = {"Authorization": "OAuth {}".format(self.mc_auth_token)}
        else:
            headers = None
        return headers

    @staticmethod
    def from_cli_args(cli_args):
        # aka requirements, serp_data, etc.
        serp_attrs = SerpAttrs.from_cli_args(cli_args)
        if cli_args.mc_auth_token_file:
            mc_auth_token = ufile.read_token_from_file(cli_args.mc_auth_token_file)
        else:
            mc_auth_token = None
        return SerpFetchParams(aspect=cli_args.mc_aspect,
                               serp_set_filter=cli_args.serp_set_filter,
                               pre_filter=cli_args.pre_filter,
                               serp_attrs=serp_attrs,
                               skip_broken=cli_args.skip_broken_serpsets,
                               retry_timeout=cli_args.mc_retry_timeout,
                               retry_count=cli_args.mc_retry_count,
                               metrics_server=cli_args.mc_server,
                               fetch_threads=cli_args.fetch_threads,
                               convert_threads=cli_args.convert_threads,
                               use_external_convertor=cli_args.use_external_convertor,
                               serpset_id_filter=cli_args.serpset_id_filter,
                               mc_auth_token=mc_auth_token)
