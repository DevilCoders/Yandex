import logging

from serp import ResMarkupInfo
from serp import ResUrlInfo
from serp import SerpQueryInfo
from serp import QueryKey
from serp import SerpMarkupInfo
from serp import SerpUrlsInfo
from serp import SerpsetParseContext  # noqa
from serp import SerpSetWriter  # noqa
from serp import ParsedSerp

from yaqlibenums import SerpComponentType
from yaqlibenums import SerpComponentAlignment


# by default, judgement values are stored in 'name' field
# in some case, we can use 'value' field to save string/number conversions, etc.

# serp = {
#   "query": {
#     "device": 0,
#     "text": "raw query text",
#     "regionId": 1,
#     "device": 0,
#     "uid": "y100500"
#   },
#   "serpInfo": {
#     "responseTime": 668,
#     "responseSize": 677093,
#     "notAnsweredHostCount": 0
#   },
#   "type": "SERP",
#   "components": [
#     {
#       "judgements.FIVE_CG_SERP_FILLED_CLICKS_FACTOR_5": {
#         "scale": "FIVE_CG_SERP_FILLED_CLICKS_FACTOR_5",
#         "name": "0.308289",
#         "value": 0.308289
#       },
#       "judgements.CLICKS_FACTOR": {
#         "scale": "CLICKS_FACTOR",
#         "name": "0.378978",
#         "value": 0.378978
#       },
#       "componentInfo": {
#         "type": 1,
#         "alignment": 3,
#         "rank": 1
#       },
#       "json.slices": [
#           "WIZLYRICS"
#       ],
#       "type": "COMPONENT",
#       "normalizations.md5": "77533689be76d983be5e3ea41d73eba8",
#       "componentUrl": {
#         "viewUrl": "view-url-text",
#         "pageUrl": "http://page.url/text"
#       }
#     }
#     ] # components
# }


class ScaleParser:
    def __init__(self, ctx, serp_component):
        """
        :type ctx: SerpsetParseContext
        :type serp_component: dict[str]
        """
        self.ctx = ctx
        required_markup_reqs = ctx.pool_parse_ctx.serp_attrs.get_full_component_req_names()

        self.scales = {}
        self.site_links = None
        for markup_key in serp_component:
            if markup_key not in required_markup_reqs:
                continue
            markup_attribute = serp_component[markup_key]
            if markup_key.startswith("judgements."):
                self.parse_judgement(markup_attribute, markup_key=markup_key)
            elif markup_key == "site-links":
                self.site_links = markup_attribute
            else:
                # generic attribute, leave 'as is'
                self.parse_generic_attribute(markup_key, markup_attribute)

    def parse_generic_attribute(self, markup_key, markup_attribute):
        """
        :type markup_key: str
        :type markup_attribute: dict
        :rtype: None
        """
        if markup_key in self.scales:
            raise Exception("Duplicate attribute {} for serpset {}".format(markup_key, self.ctx.serpset_id))
        self.scales[markup_key] = markup_attribute

    def parse_judgement(self, markup_attribute: dict, markup_key: str):
        scale_name = markup_attribute["scale"]
        if not scale_name:
            # METRICSSUPPORT-4758: use markup key as scale name, if it's empty
            scale_name = markup_key.replace("judgements.", "")
        else:
            if not markup_key.endswith(scale_name):
                logging.warning("Inconsistent scale data (see METRICSSUPPORT-4758) for scale %s", markup_key)

        use_value_field = True

        if "value" not in markup_attribute:
            use_value_field = False

        if use_value_field:
            scale_value = markup_attribute["value"]
        else:
            scale_value = markup_attribute.get("name")

        if scale_name in self.scales:
            raise Exception("Duplicate markup scale {} for serpset {}".format(scale_name, self.ctx.serpset_id))
        self.scales[scale_name] = scale_value

    def get_scales(self) -> dict:
        return self.scales

    def get_site_links(self):
        """
        :rtype: list[dict]
        """
        return self.site_links


def parse_one_serp(ctx, one_serp, qid_map, serpset_line_num, lock):
    """
    :type ctx: SerpsetParseContext
    :type one_serp: dict
    :type qid_map:
    :type lock:
    :type serpset_line_num: int | None
    :rtype: ParsedSerp
    """
    is_failed_serp = one_serp.get("metricTags.failed", False)

    raw_query = one_serp["query"]
    query_key = QueryKey.deserialize_external(raw_query)

    if is_failed_serp:
        logging.debug("failed serp: %s", query_key)

    qid = make_qid(lock, query_key, qid_map)

    query_info = SerpQueryInfo(qid=qid, query_key=query_key, is_failed_serp=is_failed_serp)

    # parse serp-level requirements: metric values, etc.
    serp_data = extract_serp_data(ctx, one_serp)

    # in Metrics' terms, serp 'component' = one search result
    components_list = one_serp.get("components", [])
    res_markups, res_urls = parse_serp_components(ctx, components_list, serpset_line_num)

    markup_info = SerpMarkupInfo(qid=qid, res_markups=res_markups, serp_data=serp_data)
    urls_info = SerpUrlsInfo(qid=qid, res_urls=res_urls)

    parsed_serp = ParsedSerp(query_info=query_info, markup_info=markup_info, urls_info=urls_info)
    return parsed_serp


def extract_serp_data(ctx, one_serp):
    """
    :type ctx: SerpsetParseContext
    :type one_serp: dict
    :rtype: dict
    """
    serp_data = {}
    serp_attrs = ctx.pool_parse_ctx.serp_attrs

    for serp_req_name in serp_attrs.get_full_serp_req_names():
        if serp_req_name in one_serp:
            serp_data[serp_req_name] = one_serp[serp_req_name]
    return serp_data


def parse_serp_components(ctx, serp_component_list, serpset_line_num):
    """
    :type ctx: SerpsetParseContext
    :type serp_component_list: list[dict]
    :type serpset_line_num: int
    :rtype: tuple[list[ResMarkupInfo], list[ResUrlInfo]]
    """
    res_markups = []
    res_urls = []

    for component_index, component in enumerate(serp_component_list):
        res_markup_info, res_url_info = parse_one_serp_component(ctx, component_index, component, serpset_line_num)

        res_markups.append(res_markup_info)
        res_urls.append(res_url_info)

    return res_markups, res_urls


def parse_one_serp_component(ctx, comp_index, component, serpset_line_num):
    """
    :type ctx:
    :type comp_index: int
    :type component:
    :type serpset_line_num
    :rtype: tuple[ResMarkupInfo, ResUrlInfo]
    """
    json_slices = component.get("json.slices", [])

    component_url = component.get("componentUrl")
    if component_url:
        # regular component
        page_url = component_url.get("pageUrl")
        view_url = component_url.get("viewUrl")
    else:
        page_url = None
        view_url = None

    component_info = component.get("componentInfo")
    if component_info:
        pos = component_info.get("rank")
        wizard_type = component_info.get("wizardType")
        is_wizard = wizard_type is not None
        alignment = component_info.get("alignment")
        result_type = component_info.get("type")
    else:
        pos = None
        wizard_type = None
        is_wizard = False
        alignment = None
        result_type = None

    if result_type is not None:
        assert result_type in SerpComponentType.ALL

    if alignment is not None:
        assert alignment in SerpComponentAlignment.ALL

    # MSTAND-1138: add FRESH(QUICK) attribute to search results
    webadd = component.get("webadd")
    if webadd:
        is_fast_robot_src = webadd.get("isFastRobotSrc", False)
    else:
        is_fast_robot_src = False

    is_turbo = component.get("tags.isTurbo", False)
    if is_turbo:
        original_url = component.get("url.originalUrl")
    else:
        original_url = None

    is_amp = component.get("tags.isAmp", False)

    coordinates = component.get("coordinates.coordinates")

    related_query_text = component.get("text.relatedQuery")

    allow_no_position = ctx.pool_parse_ctx.allow_no_position
    allow_broken_components = ctx.pool_parse_ctx.allow_broken_components

    if allow_no_position:
        # for 'ideal' serp, there is no position (components are ordered by relevance)
        if pos is None:
            pos = comp_index + 1

    # TODO: add separate field view_url?
    # currently pageUrl has priority
    # top adv block components has only viewUrls like 'site.ru'
    url = page_url or view_url

    if not allow_broken_components:
        if not related_query_text and not is_wizard:
            if url is None or pos is None:
                reason = "No wizard flag, no urls, and no 'text.relatedQuery' field"
                if not allow_no_position:
                    hint = "To process serps w/o position (e.g. ideal serps), use option '--allow-no-position'"
                else:
                    hint = "Looks like a broken serpset. To ignore this, use option '--allow-broken-components'"
                explain = "Reason: {}. Hint: {}".format(reason, hint)
                raise Exception("Bad component[{}]: ss={}, line={}. {}".format(comp_index, ctx.serpset_id,
                                                                               serpset_line_num, explain))

    scale_parser = ScaleParser(ctx, component)
    scales = scale_parser.get_scales()
    site_links = scale_parser.get_site_links()
    res_markup_info = ResMarkupInfo(pos=pos, scales=scales, site_links=site_links, is_wizard=is_wizard,
                                    wizard_type=wizard_type, related_query_text=related_query_text,
                                    is_fast_robot_src=is_fast_robot_src, coordinates=coordinates,
                                    alignment=alignment, result_type=result_type, json_slices=json_slices)
    res_url_info = ResUrlInfo(url=url, pos=pos, is_turbo=is_turbo, is_amp=is_amp, original_url=original_url)
    return res_markup_info, res_url_info


def make_qid(lock, query_key, qid_map):
    """
    :type lock:
    :type query_key: QueryKey
    :type qid_map: dict[QueryKey, int]
    :rtype: int
    """
    with lock:
        qid = qid_map.get(query_key)
        if qid:
            # already assigned
            return qid
        # not assigned => take next
        qid = len(qid_map) + 1
        qid_map[query_key] = qid
        return qid
