import itertools
import logging
import re

from typing import Optional
from typing import Set

from yaqtypes import JsonDict

import mstand_utils.baobab_helper as ubaobab
import mstand_utils.testid_helpers as utestid
import session_squeezer.referer
import session_squeezer.suggest
import session_squeezer.validation
import yaqutils.misc_helpers as umisc
import yaqutils.six_helpers as usix
import yaqutils.url_helpers as uurl


class ActionsSqueezer(object):
    """
    please set VERSION in your implementation
    """
    VERSION = None
    USE_LIBRA = True
    YT_SCHEMA = None
    SAMPLE = None
    UNWRAP_CONTAINER = True
    USE_ANY_FILTER = False

    def __init__(self):
        pass

    def get_actions(self, args):
        """
        :type args: ActionSqueezerArguments
        """
        raise NotImplementedError("ActionsSqueezer.get_actions not implemented")

    @staticmethod
    def is_testid_match(request, exp):
        """
        :type request: libra TRequest
        :type exp: ExperimentForSqueeze
        :rtype: bool
        """
        if exp.all_users or exp.all_for_history:
            return True
        if utestid.testid_is_adv(exp.testid):
            return ActionsSqueezer.has_adv_testid(request, exp)
        return request.HasTestID(exp.testid)

    @staticmethod
    def has_adv_testid(request, exp):
        """
        :type request: libra TRequest
        :type exp: ExperimentForSqueeze
        :rtype: bool
        """
        adv_test_info = request.GetAdvTestInfo() or []
        testid = utestid.convert_adv_testid(exp.testid)
        for t in adv_test_info:
            if t.TestID == testid:
                return True
        return False

    @staticmethod
    def is_filters_match(request, libra_filter):
        if not libra_filter:
            return True
        try:
            return libra_filter.Filter(request)
        except Exception as ex:
            logging.warning(
                "libra filter raised exception: %s for request %s (user %s), assuming no match",
                ex,
                request.ReqId,
                request.UID
            )
            return False

    @staticmethod
    def get_bucket(request, exp):
        """
        :type request: libra TRequest
        :type exp: ExperimentForSqueeze
        :rtype: int | None
        """
        if utestid.testid_is_adv(exp.testid):
            bucket = ActionsSqueezer.get_adv_bucket(request, exp)
        else:
            bucket = request.GetBucketByTestID(exp.testid)
        if bucket is None or bucket < 0:
            return None
        return bucket

    @staticmethod
    def get_adv_bucket(request, exp):
        """
        :type request: libra TRequest
        :type exp: ExperimentForSqueeze
        :rtype: int | None
        """
        adv_test_info = request.GetAdvTestInfo() or []
        testid = utestid.convert_adv_testid(exp.testid)
        for t in adv_test_info:
            if t.TestID == testid:
                return t.Bucket
        return None

    @staticmethod
    def check_experiments(args, request):
        """
        :type args: ActionSqueezerArguments
        :type request: libra TRequest
        :rtype ExpBucketInfo
        """
        result = ExpBucketInfo()
        for exp in args.experiments:
            result.buckets[exp] = ActionsSqueezer.get_bucket(request, exp)
            if ActionsSqueezer.is_testid_match(request, exp):
                args.result_experiments.add(exp)
                if ActionsSqueezer.is_filters_match(request, args.libra_filters.get(exp)):
                    result.matched.add(exp)
        return result

    @staticmethod
    def check_experiments_fake(args):
        """
        :type args: ActionSqueezerArguments
        :rtype ExpBucketInfo
        """
        result = ExpBucketInfo()
        for exp in args.experiments:
            args.result_experiments.add(exp)
            result.matched.add(exp)
        return result

    @staticmethod
    def check_experiments_by_testids(args, testids):
        """
        :type args: ActionSqueezerArguments
        :type testids: set[str] | dict[str, int]
        :rtype ExpBucketInfo
        """
        result = ExpBucketInfo()
        for exp in args.experiments:
            result.buckets[exp] = None
            if exp.testid in testids or exp.all_users:
                if isinstance(testids, dict) and not exp.all_users:
                    result.buckets[exp] = testids[exp.testid]
                args.result_experiments.add(exp)
                result.matched.add(exp)
        return result


class ActionSqueezerArguments(object):
    def __init__(self, container, exps, libra_filters, blockstat, day, cache_filters=None):
        """
        :type container: libra TRequestsContainer | iter[dict] | dict
        :type exps: set[ExperimentForSqueeze]
        :type libra_filters: dict[ExperimentForSqueeze]
        :type blockstat: dict[str, str]
        :type day: datetime.date
        :type cache_filters: dict[str] | None
        """
        self.container = container
        self.experiments = exps
        self.libra_filters = libra_filters
        self.blockstat = blockstat
        self.day = day
        self.result_actions = []
        self.result_experiments = set()
        self.cache_filters = cache_filters


EXP_BUCKET_FIELD = "__matched_exp_bucket"


class ExpBucketInfo(object):
    def __init__(self, buckets=None, matched=None):
        self.buckets = {} if buckets is None else buckets
        self.matched = set() if matched is None else matched

    def __repr__(self):
        return "ExpBucketInfo({!r}, {!r})".format(self.buckets, self.matched)


def parse_session_value(yuid, value):
    result = {}
    for part in value.split("\t"):
        if part:
            tokens = part.split("=", 1)
            if len(tokens) > 1:
                result[tokens[0]] = tokens[1]
            else:
                logging.error("(yuid=%s) can't parse string: %s", yuid, part)
    return result


def get_user_history(request):
    user_history = {}
    if request.IsA("TMiscRequestProperties"):
        search_props = request.SearchPropsValues
        user_history_keys = [
            "WEB.UserHistory.OldestRequestTimestamp",
            "WEB.UserHistory.RequestAmount",
            "WEB.UserHistory.RequestAmountLastWeek",
        ]
        for key in user_history_keys:
            if key in search_props:
                user_history[key] = search_props[key]

    return user_history or None


def build_request_data(base_data, request, add_request_params=False, add_web_params=False):
    """
    :type base_data: dict
    :type request: libra object
    :type add_request_params: bool
    :type add_web_params: bool
    """
    corrected_query, has_misspell = get_misspell_info(request)

    if request.IsA("TMiscRequestProperties"):
        search_props = request.SearchPropsValues
        max_relev_predict = umisc.optional_float(search_props.get("WEB.HoleDetector.relev_predict_quantile_1"))
        min_relev_predict = umisc.optional_float(search_props.get("WEB.HoleDetector.relev_predict_quantile_0"))
        user_personalization = "UPPER.Personalization.YandexUid" in search_props
        film_list_prediction = parse_float(search_props.get("WIZARD.IsFilmList.predict", ""))
    else:
        max_relev_predict = None
        min_relev_predict = None
        user_personalization = False
        film_list_prediction = None

    wiz_show, dir_show = collect_show_counters(request)
    full_request = ""
    request_params = []
    cgi = {}
    if request.IsA("TYandexRequestProperties"):
        full_request = request.FullRequest
        if add_request_params:
            url_tokens = uurl.urlparse(full_request)
            cgi = uurl.parse_qs(url_tokens.query)
            request_params = list(cgi)

    referer = session_squeezer.referer.get_referer_name(request.Referer, full_request)
    sources = collect_sources_data(request)
    wizard = collect_wizard_info(request)
    clid = collect_clid(request)

    try:
        detected_browser = request.GetBrowser()
    except Exception as ex:
        logging.warning(
            "libra GetBrowser() raised exception: %s for request %s (user %s), user agent %s",
            ex,
            request.ReqId,
            request.UID,
            request.UserAgent
        )
        detected_browser = None

    if detected_browser:
        browser, _ = detected_browser
    else:
        browser = None

    request_data = dict(
        base_data,
        ts=request.Timestamp,
        type="request",
        referer=referer,
        query=request.Query,
        correctedquery=corrected_query,
        hasmisspell=has_misspell,
        userpersonalization=user_personalization,
        maxrelevpredict=max_relev_predict,
        minrelevpredict=min_relev_predict,
        sources=sources,
        wizard=wizard,
        clid=clid,
        wiz_show=wiz_show,
        dir_show=dir_show,
        browser=browser,
    )
    if add_request_params and request.IsA("TYandexRequestProperties"):
        request_data["request_params"] = request_params
        if cgi.get("msid"):
            request_data["msid"] = cgi["msid"][0]

    user_region = None
    if request.IsA("TGeoInfoRequestProperties"):
        user_region = request.UserRegion
    if user_region is not None:
        request_data["userregion"] = user_region

    query_region = extract_query_region(full_request)
    if query_region is not None:
        request_data["queryregion"] = query_region
    if request.IsA("TYandexRequestProperties"):
        request_data["suggest"] = session_squeezer.suggest.prepare_suggest_data(request)

    if film_list_prediction is not None:
        request_data["filmlistprediction"] = film_list_prediction

    if add_web_params:
        request_data["is_permission_requested"] = collect_is_permission_requested(request)
        request_data["cluster"] = request.RearrValues.get("dssm_ctr_cluster")
        request_data["cluster_3k"] = request.RearrValues.get("qfc_id_1")
        request_data["hp_detector_predict"] = parse_float(request.RelevValues.get("hp_detector_predict", 0),
                                                          default=0.0)
        request_data["rearr_values"] = get_rearr_values(request)
        request_data["user_history"] = get_user_history(request)
        request_data["is_visible"] = request.IsVisible

        enrich_common_request_data(request, request_data)

    return request_data


def enrich_common_request_data(request: "libra.Request", request_data: JsonDict) -> None:
    request_data["ecom_classifier_prob"] = parse_float(request.RearrValues.get("wizdetection_ecom_classifier_prob"))
    request_data["ecom_classifier_result"] = parse_bool(request.RearrValues.get("wizdetection_ecom_classifier"))

    request_data["pay_detector_predict"] = parse_float(request.RelevValues.get("pay_detector_predict", 0),
                                                       default=0.0)
    request_data["query_about_one_product"] = parse_float(request.RelevValues.get("query_about_one_product", 0),
                                                          default=0.0)
    request_data["query_about_many_products"] = parse_float(request.RelevValues.get("query_about_many_products", 0),
                                                            default=0.0)


def build_click_data(base_data, click, restype, pos=None, visual_pos=None, fraud_bits=None,
                     visual_pos_ad=None, add_types=False, baobab_attrs=None, add_original_url=False,
                     placement=None, wizard_name=None, is_web=False):
    data = dict(
        base_data,
        ts=click.Timestamp,
        type="click",
        path=click.Path,
        restype=restype,
        dwelltime_on_service=click.DwellTimeOnService,
        url=click.Url,
    )
    if pos is not None:
        data["pos"] = pos
    if visual_pos is not None:
        data["visual_pos"] = visual_pos
    if visual_pos_ad is not None:
        data["visual_pos_ad"] = visual_pos_ad
    if fraud_bits is not None:
        data["fraud_bits"] = fraud_bits
    if add_types:
        data["is_dynamic_click"] = click.IsDynamic
        data["is_misc_click"] = click.IsA("TMiscResultClick")
    if baobab_attrs is not None:
        data["baobab_attrs"] = baobab_attrs
        data["baobab_path"] = baobab_attrs.get("path")
        data["block_id"] = baobab_attrs.get("block_id")
    if add_original_url:
        data["original_url"] = get_original_url(click.Url)
    if is_web and placement is not None:
        data["placement"] = placement
    if is_web and wizard_name is not None:
        data["wizard_name"] = wizard_name

    return data


def build_techevent_data(base_data, techevent):
    return dict(
        base_data,
        ts=techevent.Timestamp,
        type="techevent",
        path=techevent.Path,
        vars=techevent.Vars,
        from_block=getattr(techevent, "FromBlock", ""),
    )


def int_banner_relevance_to_float(banner_relevance):
    """
    :type banner_relevance: int | None
    :rtype: float | None
    """
    if banner_relevance is not None:
        banner_relevance /= 1000000.0
    return banner_relevance


class BlockDescription(object):
    def __init__(
            self,
            block: "libra.TBlock",
            libra_pos: int,
            restype: str,
            visual_pos: Optional[int],
            parallel: bool = False,
            relevpredict: Optional[float] = None,
            snip_len_str: Optional[float] = None,
            banner_relevance: Optional[float] = None,
            fraud_bits: Optional[int] = None,
            visual_pos_ad: Optional[int] = None,
            proximapredict: Optional[float] = None,
    ):
        self.block = block
        self.libra_pos = libra_pos
        self.restype = restype
        self.visual_pos = visual_pos
        self.parallel = parallel
        self.relevpredict = relevpredict
        self.snip_len_str = snip_len_str
        self.banner_relevance = banner_relevance
        self.fraud_bits = fraud_bits
        self.proximapredict = proximapredict

        self.main_result = self.block.GetMainResult()
        assert self.main_result
        self.pos = self.main_result.Position
        self.path = getattr(self.main_result, "Path", None)
        self.visual_pos_ad = visual_pos_ad

    def build_block_data(self):
        placement = "parallel" if self.parallel else "main"
        clicks_count = len(self.block.GetClicks())
        data = {
            "pos": self.pos,
            "visual_pos": self.visual_pos,
            "restype": self.restype,
            "placement": placement,
            "clicks_count": clicks_count,
            "height": self.block.Height,
        }
        if self.path is not None:
            data["path"] = self.path
        if self.relevpredict is not None:
            data["relevpredict"] = self.relevpredict
        if self.snip_len_str is not None:
            data["snip_len_str"] = self.snip_len_str
        if self.banner_relevance is not None:
            data["banner_relevance"] = self.banner_relevance
        if self.fraud_bits is not None:
            data["fraud_bits"] = self.fraud_bits
        if self.proximapredict is not None:
            data["proximapredict"] = self.proximapredict

        converted_path = getattr(self.main_result, "ConvertedPath", None)
        if converted_path:
            data["converted_path"] = converted_path

        url = getattr(self.main_result, "Url", None)
        if url:
            data["url"] = url

        source = getattr(self.main_result, "Source", None)
        if source:
            data["source"] = source

        base_type = getattr(self.main_result, "BaseType", None)
        if base_type:
            data["base_type"] = base_type

        name = getattr(self.main_result, "Name", None)
        if name:
            data["name"] = name

        has_thumb = getattr(self.main_result, "HasThumb", None)
        if name:
            data["has_thumb"] = has_thumb

        banner_id = getattr(self.main_result, "BannerID", None)
        if banner_id:
            data["banner_id"] = banner_id

        log_id = getattr(self.main_result, "LogID", None)
        if log_id:
            data["log_id"] = str(log_id)

        type_id = getattr(self.main_result, "TypeID", None)
        if type_id:
            data["type_id"] = type_id

        return data


def get_all_blocks(request):
    """
    :type request: libra.TRequest
    :rtype: __generator[BlockDescription]
    """

    # MSTAND-1355
    place_after_pos_dict = {}
    halfpremium = False
    if request.IsA("TBlockstatRequestProperties"):
        for block in request.GetBSBlocks():
            if block.Path == "/adv/halfpremium":
                halfpremium = True
            elif block.Path == "/direct/ad/premium/replace":
                banner_id = None
                place_after_pos = None
                for var in block.GetVars():
                    if len(var) == 2:
                        if var[0] == "-place_after_pos":
                            try:
                                place_after_pos = int(var[1])
                            except ValueError:
                                pass
                        elif var[0] == "-bannerId":
                            try:
                                banner_id = int(var[1])
                            except ValueError:
                                pass
                if banner_id and place_after_pos:
                    place_after_pos_dict[banner_id] = place_after_pos

    place_after_pos_list = []
    visual_pos = 0
    for libra_pos, block in enumerate(request.GetMainBlocks()):
        main_result = block.GetMainResult()
        if not main_result:
            continue
        if not main_result.IsA("TDirectResult"):
            continue

        fraud_bits = main_result.FraudBits
        if fraud_bits != 0:
            visual_pos_optional = None
            visual_pos_ad = None
        else:
            banner_id = main_result.BannerID
            place_after_pos = place_after_pos_dict.get(banner_id)
            if place_after_pos:
                visual_pos_ad = visual_pos + place_after_pos
                place_after_pos_list.append(place_after_pos)
            else:
                visual_pos_ad = visual_pos
            visual_pos_optional = visual_pos
        banner_relevance = int_banner_relevance_to_float(main_result.BannerRelevance)
        yield BlockDescription(
            block=block,
            libra_pos=libra_pos,
            restype=ubaobab.get_adv_type(request, block),
            visual_pos=visual_pos_optional,
            visual_pos_ad=visual_pos_ad,
            banner_relevance=banner_relevance,
            fraud_bits=fraud_bits,
        )
        if fraud_bits == 0:
            visual_pos += 1

    organic_pos = 0
    for libra_pos, block in enumerate(request.GetMainBlocks()):
        main_result = block.GetMainResult()
        if not main_result:
            continue
        if main_result.IsA("TDirectResult"):
            continue

        relevpredict = None
        snip_len_str = None
        proximapredict = None
        if main_result.IsA("TWizardResult") or main_result.IsA("TBlenderWizardResult"):
            restype = "wiz"
        elif main_result.IsA("TWebResult"):
            restype = "web"
            relevpredict = main_result.Markers.get("RelevPrediction")
            relevpredict = umisc.optional_float(relevpredict)
            snip_len_str = main_result.Markers.get("SnipLenStr")
            snip_len_str = umisc.optional_float(snip_len_str)
            proximapredict = umisc.optional_float(main_result.Markers.get("ProximaPredict"))
        elif main_result.IsA("TRecommendationResult"):
            restype = "recommendation"
        else:
            restype = "other"

        visual_pos_ad = visual_pos
        for place_after_pos in place_after_pos_list:
            if place_after_pos > organic_pos:
                visual_pos_ad -= 1

        yield BlockDescription(
            block=block,
            libra_pos=libra_pos,
            restype=restype,
            visual_pos=visual_pos,
            visual_pos_ad=visual_pos_ad,
            relevpredict=relevpredict,
            snip_len_str=snip_len_str,
            proximapredict=proximapredict,
        )
        organic_pos += 1
        visual_pos += 1

    for libra_pos, block in enumerate(request.GetParallelBlocks()):
        main_result = block.GetMainResult()
        if main_result and main_result.IsA("TDirectResult"):
            fraud_bits = main_result.FraudBits
            if fraud_bits == 0 and halfpremium:
                visual_pos_optional = visual_pos
            else:
                visual_pos_optional = None
            banner_relevance = int_banner_relevance_to_float(main_result.BannerRelevance)
            yield BlockDescription(
                block=block,
                libra_pos=libra_pos,
                restype="guarantee",
                visual_pos=visual_pos_optional,
                visual_pos_ad=visual_pos_optional,
                parallel=True,
                banner_relevance=banner_relevance,
                fraud_bits=fraud_bits,
            )
            if fraud_bits == 0:
                visual_pos += 1

        if main_result and main_result.IsA("TWizardResultProperties"):
            yield BlockDescription(
                block=block,
                libra_pos=libra_pos,
                restype="wiz_parallel",
                visual_pos=None,  # skip visual pos for wizards in parallel blocks MSTAND-1242
                parallel=True,
            )


def get_click_fraud_bits(click):
    direct_properties = click.GetProperties("TDirectClickProperties")
    if direct_properties:
        return direct_properties.FraudBits
    return None


def get_original_url(url):
    if not url:
        return None

    if usix.is_arcadia():
        # noinspection PyUnresolvedReferences,PyPackageRequirements
        from quality.functionality.turbo.urls_lib.python.lib import bindings
    else:
        # noinspection PyUnresolvedReferences,PyPackageRequirements
        import bindings

    decoded_turbo_url = bindings.try_decode_turbo_url(url.encode("utf-8"))
    if decoded_turbo_url:
        return decoded_turbo_url.decode("utf-8")

    return url


def append_clicks(known_actions, base_data, request, ad_replace=False, add_baobab_attrs=False, add_original_url=False,
                  is_web=False):
    baobab_helper = ubaobab.BaobabHelper(request)
    for description in get_all_blocks(request):
        for click in description.block.GetClicks():
            fraud_bits = get_click_fraud_bits(click)
            click_data = build_click_data(
                base_data=base_data,
                click=click,
                restype=description.restype,
                pos=description.pos,
                visual_pos=description.visual_pos,
                visual_pos_ad=description.visual_pos_ad if ad_replace else None,
                fraud_bits=fraud_bits,
                baobab_attrs=baobab_helper.get_click_attrs(click),
                add_original_url=add_original_url,
                placement="parallel" if description.parallel else "main",
                wizard_name=getattr(description.main_result, "Name", None),
                is_web=is_web,
            )
            known_actions.append(click_data)

    # dynamic clicks
    if request.IsA("TWebRequestProperties"):
        for click in request.GetClicks():
            if click.IsDynamic:
                click_data = build_dynamic_click_data(base_data, click)
                if add_baobab_attrs:
                    click_data["baobab_attrs"] = baobab_helper.get_click_attrs(click)
                known_actions.append(click_data)


TECHEVENT_PATHS = {
    "143.768",  # page/scroll
    "690.405.487",  # tech/pager/show
    "690.1033",  # tech/timing
    "thumbs-preview.animation",
    "$page/$main/related/shiny_discovery",
    "$page/$main/related/shiny_discovery_above",
}


def append_techevents(known_actions, base_data, request, add_baobab_attrs=False):
    baobab_helper = ubaobab.BaobabHelper(request)
    for techevent in request.GetYandexTechEvents():
        b_tech_attrs = baobab_helper.get_tech_attrs(techevent) if add_baobab_attrs else None
        if techevent.Path in TECHEVENT_PATHS or b_tech_attrs and b_tech_attrs["path"] in TECHEVENT_PATHS:
            techevent_data = build_techevent_data(
                base_data=base_data,
                techevent=techevent
            )
            if add_baobab_attrs:
                techevent_data["baobab_attrs"] = b_tech_attrs
            known_actions.append(techevent_data)


def append_misc_clicks(known_actions, base_data, request, add_original_url=False):
    baobab_helper = ubaobab.BaobabHelper(request)
    for click in request.GetMiscClicks():
        if click.IsA("TMiscResultClick") and not click.IsDynamic:
            click_data = build_click_data(
                base_data,
                click,
                restype="misc",
                baobab_attrs=baobab_helper.get_click_attrs(click),
                add_original_url=add_original_url,
            )
            if click.Path == "65.66":
                click_data["target"] = "google_search"
            elif click.Path == "65.176":
                click_data["target"] = "mail_search"
            elif click.Path == "65.568":
                click_data["target"] = "bing_search"
            known_actions.append(click_data)


RE_CLID = re.compile(r"clid=(?P<clid>\d+)")


def collect_clid(request):
    if request.IsA("TYandexRequestProperties"):
        match = RE_CLID.search(request.FullRequest)
        if match is not None:
            return match.group("clid")
    return ""


def get_misspell_info(request):
    if request.IsA("TMisspellRequestProperties"):
        return request.CorrectedQuery, bool(request.IsMSP)
    else:
        return "", False


def build_dynamic_click_data(base_data, click):
    return dict(
        base_data,
        ts=click.Timestamp,
        type="dynamic-click",
        path=click.Path,
        url=click.Url,
        dwelltime_on_service=click.DwellTimeOnService,
    )


def collect_sources_data(request):
    sources = []
    for blk in request.GetMainBlocks():
        main_result = blk.GetMainResult()
        if not main_result:
            continue
        if main_result.IsA("TWebResult") or main_result.IsA("TMapsResult"):
            sources.append(main_result.Source)
    return sources


def collect_show_counters(request):
    wiz_show = 0
    dir_show = 0

    # Show count
    for blk in request.GetMainBlocks():
        main_result = blk.GetMainResult()
        if not main_result:
            continue
        # TODO: same as visual_pos?
        if main_result.IsA("TWizardResult"):
            wiz_show += 1
        if main_result.IsA("TDirectResult"):
            dir_show += 1

    return wiz_show, dir_show


def collect_wizard_info(request):
    if not request.IsA("TYandexRequestProperties"):
        return False
    # next logic doesn"t support video wizard at all
    if "&source=wiz" in request.FullRequest:  # correct for images only
        return True
    if request.IsA("TMapsRequestProperties"):
        origin = request.Origin
        return origin == "maps-wizgeo" or origin == "maps-wizbiz-new"  # correct for maps only
    return False


def collect_is_permission_requested(request):
    """
    :type request: libra.TRequest
    :rtype: bool
    """
    return any("690.471.287.614.1304" in techevent.Path
               for techevent in request.GetYandexTechEvents())  # MSTAND-1446


# assume, that query region should be lower than 10^18 (to fit in int64)
MAXIMUM_REGION_LENGTH = 18


def extract_query_region(full_query):
    if not full_query:
        return None
    lr_index = full_query.find("lr=")
    if lr_index < 0:
        return None
    region = ""
    for c in full_query[(lr_index + 3):]:
        if c.isdigit():
            region += c
        else:
            break
    if not region:
        return None
    if len(region) > MAXIMUM_REGION_LENGTH:
        return None
    return int(region)


def resolve_dom_region_from_host(host):
    """
    :type host: str
    :rtype: str
    """
    return host.rsplit(".", 1)[1]


def get_dom_region(request):
    if request.IsA("TYandexRequestProperties"):
        return request.ServiceDomRegion
    if request.IsA("TPortalRequestProperties"):
        return resolve_dom_region_from_host(request.Host)
    return None


def prepare_base_data(exp_bucket, request, add_ui=False):
    """
    :type exp_bucket: ExpBucketInfo
    :type request: libra.TRequest
    :type add_ui: bool
    :rtype: dict[str]
    """
    base_data = {
        EXP_BUCKET_FIELD: exp_bucket,
    }
    dom_region = get_dom_region(request)
    if dom_region is not None:
        base_data["domregion"] = dom_region
    if hasattr(request, "ReqId"):
        base_data["reqid"] = request.ReqId
    if request.IsA("TPageProperties"):
        base_data["page"] = request.PageNo
    if add_ui:
        if request.IsA("TDesktopUIProperties"):
            base_data["ui"] = "desktop"
        elif request.IsA("TTouchUIProperties"):
            base_data["ui"] = "touch"
        elif request.IsA("TPadUIProperties"):
            base_data["ui"] = "pad"
        elif request.IsA("TMobileAppUIProperties"):
            base_data["ui"] = "mobileapp"
        elif request.IsA("TMobileUIProperties"):
            base_data["ui"] = "mobile"
    return base_data


def get_kinopoisk_actions(args: ActionSqueezerArguments,
                          column_names: Set[str],
                          testids_field_name: str,
                          convert_timestamp: bool = False) -> None:
    all_testids = set()
    for group_type, group in itertools.groupby(args.container, key=lambda x: x["table_index"]):
        if group_type == 0:
            for row in group:
                all_testids.update(row["experiments"])
                break
        else:
            for row in group:
                squeezed = {key: value for key, value in row.items() if key in column_names}
                squeezed["ts"] = row["timestamp"] // 1000 if convert_timestamp else row["timestamp"]
                if row[testids_field_name] is not None:
                    all_testids.update(row[testids_field_name].split(","))
                exp_bucket = ActionsSqueezer.check_experiments_by_testids(args, all_testids)
                squeezed[EXP_BUCKET_FIELD] = exp_bucket
                args.result_actions.append(squeezed)


MAX_INT64 = 2 ** 63 - 1


def positive_int64_or_none(val, val_description):
    """
    :type val: int | None
    :type val_description: str
    :rtype: int | None
    """
    if val is None:
        return val

    if 0 <= val <= MAX_INT64:
        return val
    else:
        logging.warning("Value %s has set to None: %s is not positive int64", val_description, val)
        return None


def fix_positive_int64_recursive(val, val_description):
    """
    :type val: int | float | list | tuple | dict | None
    :type val_description: str
    :rtype: int | None
    """
    if umisc.is_integer(val):
        return positive_int64_or_none(val, val_description)
    if isinstance(val, (list, tuple)):
        return [fix_positive_int64_recursive(x, val_description) for i, x in enumerate(val)]
    if isinstance(val, dict):
        return {fix_positive_int64_recursive(k, val_description): fix_positive_int64_recursive(v, val_description)
                for k, v in usix.iteritems(val)}
    return val


def parse_triplet(triplet, yuid=None):
    """
    :param triplet: str
    :param yuid: str | None
    :return: tuple[str, int] | None
    """
    try:
        if "," in triplet:
            testid, _, bucket = triplet.split(",")
        else:
            testid, bucket = triplet.split(":")
        return testid, int(bucket)
    except ValueError:
        logging.warning("Could not parse the triplet: %s, yuid: %s", triplet, yuid)
    except Exception:
        logging.error("Unknown error parsing triplet: %s, yuid: %s", triplet, yuid)
        raise


def get_testids(rows, testids_column_name):
    """
    :type rows: iter[dict[str]]
    :type testids_column_name: str
    :rtype: dict[str, int]
    """
    for row in rows:
        if row[testids_column_name]:
            return dict(
                testid_to_bucket
                for testid_to_bucket in map(parse_triplet, row[testids_column_name].split(";"))
                if testid_to_bucket
            )


def is_bro_testids(experiments):
    return any(utestid.testid_is_bro(exp.testid) for exp in experiments)


def get_rearr_values(request):
    keys = ["IsFinancialQuery", "IsLawQuery", "IsMedicalQuery"]
    result = {}
    for k in keys:
        if k in request.RearrValues:
            fvalue = parse_float(request.RearrValues[k])
            if fvalue:
                result[k] = fvalue
    return result


def parse_float(value: str, default: Optional[float] = None) -> Optional[float]:
    try:
        return float(value)
    except (ValueError, TypeError):
        return default


def parse_bool(value: Optional[str]) -> Optional[bool]:
    if value is None:
        return None
    return value == "1"
