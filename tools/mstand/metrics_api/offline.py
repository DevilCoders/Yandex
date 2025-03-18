import mstand_utils.mstand_misc_helpers as mstand_umisc
from metrics_api import ExperimentForAPI  # noqa
from metrics_api import ObservationForAPI  # noqa

from yaqlibenums import SerpComponentType
from yaqlibenums import SerpComponentAlignment

from serp import LevelOneScaleStats
from serp import LevelTwoScaleStats
from serp import ResMarkupInfo  # noqa
from serp import ResUrlInfo  # noqa
from serp import SerpMarkupInfo  # noqa
from serp import SerpQueryInfo  # noqa
from serp import SerpUrlsInfo  # noqa

# NOTICE: Classes below are used directly in user metrics.
# Interface/fields should NOT be changed/renamed/etc without 'mstand community' approval.

ResultTypeEnumForAPI = SerpComponentType
AlignmentEnumForAPI = SerpComponentAlignment


class ResultInfoForAPI(object):
    def __init__(self, res_markup: ResMarkupInfo, res_url: ResUrlInfo, index: int, collect_scale_stats: bool):
        if res_url is not None:
            self.url = res_url.url
            self.host = mstand_umisc.extract_host_from_url(self.url, normalize=False)
            self.normalized_host = mstand_umisc.extract_host_from_url(self.url, normalize=True)
            self.is_turbo = res_url.is_turbo
            self.is_amp = res_url.is_amp
            self.original_url = res_url.original_url
            self.original_host = mstand_umisc.extract_host_from_url(self.original_url, normalize=False)
            self.original_normalized_host = mstand_umisc.extract_host_from_url(self.original_url, normalize=True)
        else:
            # don't add attributes with None values to avoid misuse without --load-urls parameter.
            pass

        self.scales = res_markup.scales
        self.site_links = res_markup.site_links

        self.related_query_text = res_markup.related_query_text
        # use 'real' component position
        self.index = index
        self.pos = index + 1
        # original 'rank' field from Metrics
        self.orig_pos = res_markup.pos
        # MSTAND-1078: support wizard attributes
        self.is_wizard = res_markup.is_wizard
        self.wizard_type = res_markup.wizard_type
        # MSTAND-1138: support FRESH(QUICK) attributes
        self.is_fast_robot_src = res_markup.is_fast_robot_src
        # MSTAND-1273: support coordinates for geo metrics
        self.coordinates = res_markup.coordinates
        # MSTAND-1170: support alignment
        self.alignment = res_markup.alignment
        # MSTAND-1286: support result type
        self.result_type = res_markup.result_type
        # MSTAND-1595: support json.slices
        self.json_slices = res_markup.json_slices

        self.scale_stat_depth = None
        self.collect_scale_stats = collect_scale_stats

        self.scale_stats = LevelOneScaleStats() if collect_scale_stats else None

    def init_scale_stat_depth(self, scale_stat_depth):
        self.scale_stat_depth = scale_stat_depth

    def do_collect_stats(self):
        """
        :rtype: bool
        """
        return self.scale_stat_depth is None or self.index < self.scale_stat_depth

    def has_scale(self, scale_name):
        if self.collect_scale_stats and self.do_collect_stats():
            return self.scale_stats.check_scale(scale_name, self.scales)
        else:
            return scale_name in self.scales

    def get_scale(self, scale_name, default=None):
        if self.collect_scale_stats and self.do_collect_stats():
            self.scale_stats.check_scale(scale_name, self.scales)
        return self.scales.get(scale_name, default)

    def get_scale_strict(self, scale_name):
        if self.collect_scale_stats and self.do_collect_stats():
            self.scale_stats.check_scale(scale_name, self.scales)
        else:
            return self.scales[scale_name]


class SerpMetricParamsForAPI(object):
    def __init__(self, query_info, markup_info, urls_info, observation, experiment, collect_scale_stats):
        """
        :type query_info: SerpQueryInfo
        :type markup_info: SerpMarkupInfo
        :type urls_info: SerpUrlsInfo
        :type observation: ObservationForAPI
        :type experiment: ExperimentForAPI
        """
        self.observation = observation
        self.experiment = experiment
        # internal mstand's query ID
        self.qid = query_info.qid
        self.query_text = query_info.query_key.query_text
        self.query_region = query_info.query_key.query_region
        self.query_device = query_info.query_key.query_device
        self.query_uid = query_info.query_key.query_uid
        self.query_country = query_info.query_key.query_country
        self.query_map_info = query_info.query_key.query_map_info

        self.results = []
        if urls_info is not None:
            assert len(markup_info.res_markups) == len(urls_info.res_urls)
        for rindex, res_markup in enumerate(markup_info.res_markups):
            if urls_info is not None:
                res_url = urls_info.res_urls[rindex]
            else:
                res_url = None
            result_info_api = ResultInfoForAPI(res_markup=res_markup, res_url=res_url, index=rindex,
                                               collect_scale_stats=collect_scale_stats)
            self.results.append(result_info_api)

        self.serp_data = markup_info.serp_data

        self.collect_scale_stats = collect_scale_stats
        self.serp_data_stats = LevelOneScaleStats() if collect_scale_stats else None

    def has_serp_data(self, field_name):
        return self.serp_data_stats.check_scale(field_name, self.serp_data)

    def get_serp_data(self, field_name, default=None):
        if self.collect_scale_stats:
            self.serp_data_stats.check_scale(field_name, self.serp_data)
        return self.serp_data.get(field_name, default)

    def get_serp_data_strict(self, field_name):
        if self.collect_scale_stats:
            self.serp_data_stats.check_scale(field_name, self.serp_data)
        return self.serp_data[field_name]


class SerpMetricParamsByPositionForAPI(object):
    def __init__(self, query_info: SerpQueryInfo, markup_info: SerpMarkupInfo, urls_info: SerpUrlsInfo,
                 index: int, result: ResultInfoForAPI, observation: ObservationForAPI, experiment: ExperimentForAPI,
                 collect_scale_stats: bool):
        self.observation = observation
        self.experiment = experiment
        # internal mstand's query ID
        self.qid = query_info.qid
        self.query_text = query_info.query_key.query_text
        self.query_region = query_info.query_key.query_region
        self.query_device = query_info.query_key.query_device
        self.query_uid = query_info.query_key.query_uid
        self.query_country = query_info.query_key.query_country
        self.query_map_info = query_info.query_key.query_map_info
        self.urls_info = urls_info

        self.serp_data = markup_info.serp_data

        self.index = index
        self.pos = index + 1

        self.result = result
        self.collect_scale_stats = collect_scale_stats
        self.serp_data_stats = LevelOneScaleStats() if self.collect_scale_stats else None

    def has_serp_data(self, field_name):
        return self.serp_data_stats.check_scale(field_name, self.serp_data)

    def get_serp_data(self, field_name, default=None):
        self.serp_data_stats.check_scale(field_name, self.serp_data)
        return self.serp_data.get(field_name, default)

    def get_serp_data_strict(self, field_name):
        self.serp_data_stats.check_scale(field_name, self.serp_data)
        return self.serp_data[field_name]


class SerpMetricAggregationParamsForAPI(object):
    def __init__(self, mparams_api: SerpMetricParamsForAPI, metric_values: 'SerpMetricValuesForAPI'):
        self.observation = mparams_api.observation
        self.experiment = mparams_api.experiment
        # internal mstand's query ID
        self.qid = mparams_api.qid
        self.query_text = mparams_api.query_text
        self.query_region = mparams_api.query_region
        self.query_device = mparams_api.query_device
        self.query_uid = mparams_api.query_uid
        self.query_country = mparams_api.query_country
        self.query_map_info = mparams_api.query_map_info

        self.serp_data = mparams_api.serp_data

        self.pos_metric_values = metric_values.values_by_position
        self.results = mparams_api.results

        self.serp_data_stats = LevelOneScaleStats()

    def has_serp_data(self, field_name):
        return self.serp_data_stats.check_scale(field_name, self.serp_data)

    def get_serp_data(self, field_name, default=None):
        self.serp_data_stats.check_scale(field_name, self.serp_data)
        return self.serp_data.get(field_name, default)

    def get_serp_data_strict(self, field_name):
        self.serp_data_stats.check_scale(field_name, self.serp_data)
        return self.serp_data[field_name]


class SerpMetricValueByPositionForAPI(object):
    def __init__(self, value, index):
        self.value = value
        self.index = index
        self.pos = index + 1

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "(val[{pos}]={val})".format(val=self.value, pos=self.pos)


class SerpMetricValuesForAPI(object):
    def __init__(self, total_value=None, values_by_position=None, has_details=False, has_error=False,
                 error_message=None):
        """
        :type total_value: float | None
        :type values_by_position: list[SerpMetricValueByPositionForAPI] | None
        :type has_details: bool
        :type has_error: bool
        :type error_message: str | None
        """
        self.total_value = total_value
        self.values_by_position = values_by_position or []
        self.has_details = has_details
        self.has_error = has_error
        self.error_message = error_message

        # lazy init
        self.scale_stats = None
        self.serp_data_stats = None

    def add_position_value(self, value_by_position):
        """
        :type value_by_position: SerpMetricValueByPositionForAPI
        :rtype
        """
        self.values_by_position.append(value_by_position)

    def __str__(self):
        return "total={}, vals_by_pos={}".format(self.total_value, self.values_by_position)

    def update_scale_stats(self, level_one_stats):
        """
        :type level_one_stats: LevelOneScaleStats
        :rtype:
        """
        if self.scale_stats is None:
            self.scale_stats = LevelTwoScaleStats()

        self.scale_stats.update_stats(level_one_stats)

    def update_serp_data_stats(self, level_one_stats):
        """
        :type level_one_stats: LevelOneScaleStats
        :rtype:
        """
        if self.serp_data_stats is None:
            self.serp_data_stats = LevelTwoScaleStats()
        self.serp_data_stats.update_stats(level_one_stats)
