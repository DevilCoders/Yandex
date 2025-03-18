import logging
from functools import partial
import gzip
from numbers import Real
import os
import time
import traceback
import types
import math

import serp.offline_metric_caps as om_caps
import yaqutils.file_helpers as ufile
import yaqutils.json_helpers as ujson
import yaqutils.tsv_helpers as utsv
import yaqutils.misc_helpers as umisc
import yaqutils.six_helpers as usix
from experiment_pool import MetricDataType
from experiment_pool import MetricValues
from experiment_pool import Experiment
from experiment_pool import Observation
from metrics_api.offline import ResultInfoForAPI
from metrics_api.offline import SerpMetricAggregationParamsForAPI
from metrics_api.offline import SerpMetricParamsByPositionForAPI
from metrics_api.offline import SerpMetricParamsForAPI
from metrics_api.offline import SerpMetricValueByPositionForAPI
from metrics_api.offline import SerpMetricValuesForAPI

from pytlib import YtApi

from mstand_utils import OfflineDefaultValues

from serp import OfflineComputationCtx  # noqa
from serp import OfflineMetricCtx  # noqa
from serp import ScaleStats
from serp import SerpMarkupInfo  # noqa
from serp import SerpQueryInfo  # noqa
from serp import ParsedSerp

from serp import SerpSetFileHandler
from serp import SerpSetReader

from serp import ExtMetricResultsWriter
from serp import ExtMetricValue
from serp import ExtMetricResultsTable

from serp import SerpUrlsInfo  # noqa
from serp import SerpMetricValue
from serp import SerpsetMetricValues


def compute_metric_by_serpset(comp_ctx: OfflineComputationCtx):
    met_ctx = comp_ctx.metric_ctx
    global_ctx = met_ctx.global_ctx
    serp_storage = met_ctx.parsed_serp_storage

    queries_file = serp_storage.queries_by_serpset(comp_ctx.serpset_id)
    markup_file = serp_storage.markup_by_serpset(comp_ctx.serpset_id)

    if comp_ctx.load_urls:
        urls_file = serp_storage.urls_by_serpset(comp_ctx.serpset_id)
    else:
        urls_file = None

    metric_storage = met_ctx.metric_storage

    start_time = time.time()

    serpset_file_handler = SerpSetFileHandler(queries_file=queries_file, markup_file=markup_file,
                                              urls_file=urls_file, mode="read")

    metric_file_external = metric_storage.metric_by_serpset_external(comp_ctx.serpset_id)

    if global_ctx.gzip_external_output:
        metric_file_external = "{}.gz".format(metric_file_external)
        # MSTAND-1851: text mode in gzip allows to write gzip as text (str, not bytes)
        write_func = partial(gzip.open, metric_file_external, mode="w+t", compresslevel=6)
    else:
        write_func = partial(ufile.fopen_write, metric_file_external)

    with serpset_file_handler:
        with write_func() as metric_ext_fd:
            serpset_reader = SerpSetReader.from_file_handler(serpset_file_handler)
            metric_keys = met_ctx.plugin_container.plugin_key_map
            ext_writer = ExtMetricResultsWriter(ext_res_fd=metric_ext_fd, metric_keys=metric_keys,
                                                mc_alias_prefix=global_ctx.mc_alias_prefix,
                                                mc_error_alias_prefix=global_ctx.mc_error_alias_prefix)

            compute_metrics_on_serpset(comp_ctx=comp_ctx, serpset_reader=serpset_reader,
                                       ext_writer=ext_writer)

    umisc.log_elapsed(start_time, "serpset %s calculated", comp_ctx.serpset_id)


def parse_and_aggregate_metric_values(detailed_result, metric_file):
    """
    :type detailed_result: list[list[str]]
    :type metric_file: str
    :rtype: MetricValues
    """
    row_count = len(detailed_result)
    data_file = os.path.basename(metric_file)

    number_count = 0
    number_sum = 0
    for str_row in detailed_result:
        row_values = [ujson.load_from_str(str_value) for str_value in str_row]
        for value in row_values:
            if isinstance(value, Real):
                number_sum += value
                number_count += 1

    if number_count > 0:
        number_average = float(number_sum) / number_count
    else:
        number_average = None

    # all rows are expected to be numeric now
    return MetricValues(count_val=number_count,
                        sum_val=number_sum,
                        average_val=number_average,
                        data_type=MetricDataType.KEY_VALUES,
                        data_file=data_file,
                        row_count=row_count)


def aggregate_metric_by_serpset(comp_ctx):
    """
    :type comp_ctx: OfflineComputationCtx
    :rtype: dict[int, MetricValues]
    """

    metric_container = comp_ctx.metric_ctx.plugin_container
    metric_data_storage = comp_ctx.metric_ctx.metric_storage
    serpset_id = comp_ctx.serpset_id

    logging.info("Aggregating offline metrics for serpset %s", serpset_id)

    agg_results = {}

    if not comp_ctx.metric_ctx.global_ctx.use_internal_output:
        logging.warning("No internal format required, skipping aggregation for serpset %s", serpset_id)
        return agg_results

    for metric_id in metric_container.plugin_instances:
        if metric_id not in comp_ctx.error_metrics:
            metric_key = metric_container.plugin_key_map[metric_id]
            logging.info("Processing metric %s with id %d", metric_key, metric_id)

            metric_file = metric_data_storage.metric_by_serpset(metric_id, serpset_id)
            detailed_results = read_raw_metric_results(metric_file)

            agg_results[metric_id] = parse_and_aggregate_metric_values(detailed_results, metric_file)
    return agg_results


def read_raw_metric_results(metric_data_file):
    """
    :type metric_data_file: str
    :rtype: list[list[str]]
    """
    detailed_results = []
    with ufile.fopen_read(metric_data_file) as metric_fd:
        for line in metric_fd:
            tokens = line.rstrip("\n").split("\t")
            # offline metric values are key-valued => skip key
            metric_values = tokens[1:]
            detailed_results.append(metric_values)
    return detailed_results


def compute_metrics_on_serpset(comp_ctx, serpset_reader, ext_writer):
    """
    :type comp_ctx: OfflineComputationCtx
    :type serpset_reader: SerpSetReader
    :type ext_writer: ExtMetricResultsWriter
    :rtype:
    """
    serpset_id = comp_ctx.serpset_id

    metric_start_time = time.time()
    logging.info("Metric calculation started for serpset id %s", serpset_id)

    serpset_metric_vals = SerpsetMetricValues(use_int_vals=comp_ctx.use_int_out)

    scale_stats = ScaleStats()
    processed_query_count = 0
    total_res_count = 0
    for parsed_serp in serpset_reader:
        try:
            if parsed_serp.query_info.is_failed_serp and comp_ctx.skip_failed_serps:
                logging.debug("serp %s is failed", parsed_serp.query_info)
                ext_metric_vals = {}
                serp_depth = 0
            else:
                serp_metric_vals_iter = compute_metrics_on_serp(comp_ctx=comp_ctx, parsed_serp=parsed_serp)
                for smv in serp_metric_vals_iter:
                    serpset_metric_vals.add_metric_value(qid=smv.qid, metric_id=smv.metric_id, one_metric_val=smv.one_metric_val)
                    if comp_ctx.metric_ctx.global_ctx.collect_scale_stats:
                        scale_stats.update_stats(metric_id=smv.metric_id, one_metric_val=smv.one_metric_val)

                serp_depth = parsed_serp.markup_info.result_count()
                ext_metric_vals = serpset_metric_vals.query_ext_metric_vals

            ext_writer.write_one_query(query_info=parsed_serp.query_info, serp_depth=serp_depth, ext_metric_vals=ext_metric_vals)

            processed_query_count += 1
            total_res_count += serp_depth
            if processed_query_count % 1000 == 0:
                logging.info("%s queries processed", processed_query_count)
        except Exception as exc:
            trace = traceback.format_exc()
            logging.error("Error in metrics computation on query %s: %s, details: %s", processed_query_count, exc, trace)
            raise

    umisc.log_elapsed(metric_start_time, "Metric computation for serpset %s done", serpset_id)

    ext_writer.terminate()

    avg_serp_size = float(total_res_count) / processed_query_count if processed_query_count else 0.0
    logging.info("Average SERP size: %.2f results per query", avg_serp_size)

    scale_stats.log_stats(serpset_id, processed_query_count)
    dump_serpset_metric_results(comp_ctx, serpset_metric_vals)


def dump_serpset_metric_results(comp_ctx, serpset_metric_vals):
    """
    :type comp_ctx: OfflineComputationCtx
    :type serpset_metric_vals: SerpsetMetricValues
    :rtype:
    """
    if comp_ctx.use_int_out:
        logging.info("Dumping metric results on serpset %s [internal]", comp_ctx.serpset_id)
        dump_metric_values_internal(comp_ctx, serpset_metric_vals)
    else:
        logging.info("Skipping internal dump for serpset %s", comp_ctx.serpset_id)


def compute_metrics_on_serp(comp_ctx, parsed_serp):
    """
    :type comp_ctx: OfflineComputationCtx
    :type parsed_serp: ParsedSerp
    :rtype: iter[SerpMetricValue]
    """
    # common metrics params for value() and precompute()
    mparams_api = SerpMetricParamsForAPI(query_info=parsed_serp.query_info,
                                         markup_info=parsed_serp.markup_info,
                                         urls_info=parsed_serp.urls_info,
                                         observation=comp_ctx.observation,
                                         experiment=comp_ctx.experiment,
                                         collect_scale_stats=comp_ctx.collect_scale_stats)

    mparams_by_pos_api_list = build_detailed_metric_params(comp_ctx, query_info=parsed_serp.query_info,
                                                           markup_info=parsed_serp.markup_info,
                                                           urls_info=parsed_serp.urls_info)
    return compute_batch_on_one_serp(comp_ctx=comp_ctx,
                                     mparams_api=mparams_api,
                                     mparams_by_pos_api_list=mparams_by_pos_api_list)


def build_detailed_metric_params(comp_ctx, query_info, markup_info, urls_info):
    """
    :type comp_ctx: OfflineComputationCtx
    :type query_info: SerpQueryInfo
    :type markup_info: SerpMarkupInfo
    :type urls_info: SerpUrlsInfo
    :rtype: list[SerpMetricParamsByPositionForAPI] | None
    """
    if not comp_ctx.has_detailed:
        return None

    mparams_by_pos_api_list = []

    for rindex, res_markup in enumerate(markup_info.res_markups):
        if comp_ctx.max_serp_depth is not None:
            if rindex >= comp_ctx.max_serp_depth:
                break

        if urls_info is not None:
            res_url = urls_info.res_urls[rindex]
        else:
            res_url = None
        result_info_api = ResultInfoForAPI(res_markup=res_markup, res_url=res_url, index=rindex,
                                           collect_scale_stats=comp_ctx.collect_scale_stats)

        params_by_pos_api = SerpMetricParamsByPositionForAPI(query_info=query_info,
                                                             markup_info=markup_info,
                                                             urls_info=urls_info,
                                                             index=rindex,
                                                             result=result_info_api,
                                                             observation=comp_ctx.observation,
                                                             experiment=comp_ctx.experiment,
                                                             collect_scale_stats=comp_ctx.collect_scale_stats)
        mparams_by_pos_api_list.append(params_by_pos_api)
    return mparams_by_pos_api_list


def compute_batch_on_one_serp(comp_ctx, mparams_api, mparams_by_pos_api_list):
    """
    :type comp_ctx: OfflineComputationCtx
    :type mparams_api: SerpMetricParamsForAPI
    :type mparams_by_pos_api_list: list
    :rtype: iter[SerpMetricValue]
    """
    cont = comp_ctx.metric_ctx.plugin_container
    for metric_id, metric_instance in usix.iteritems(cont.plugin_instances):
        one_metric_val = compute_one_metric_value(metric_id=metric_id,
                                                  metric_instance=metric_instance,
                                                  mparams_api=mparams_api,
                                                  mparams_by_pos_api_list=mparams_by_pos_api_list,
                                                  comp_ctx=comp_ctx)

        if one_metric_val.has_error:
            # MSTAND-1339, MSTAND-1186
            comp_ctx.error_metrics.add(metric_id)

        serp_metric_value = SerpMetricValue(qid=mparams_api.qid, metric_id=metric_id, one_metric_val=one_metric_val)
        yield serp_metric_value


def compute_one_metric_value(metric_id, metric_instance, mparams_api, mparams_by_pos_api_list, comp_ctx):
    """
    :type metric_id: int
    :type metric_instance:
    :type mparams_api: SerpMetricParamsForAPI
    :type mparams_by_pos_api_list: list[SerpMetricParamsByPositionForAPI] | None
    :type comp_ctx: OfflineComputationCtx
    :rtype: SerpMetricValuesForAPI
    """

    is_local_calc_mode = comp_ctx.metric_ctx.global_ctx.is_local_calc_mode()

    if om_caps.is_detailed_metric(metric_instance):
        assert mparams_by_pos_api_list is not None
        scale_stat_depth = om_caps.get_metric_scale_stat_depth(metric_instance)

        try:
            # measure time precompute + detailed
            # optional precompute stage
            if om_caps.has_precompute(metric_instance):

                precomp_start_time = time.time()
                metric_instance.precompute(mparams_api)
                if is_local_calc_mode:
                    comp_ctx.accumulate_metric_calc_time(metric_id, precomp_start_time)

            metric_vals_api = compute_metric_detailed(comp_ctx=comp_ctx,
                                                      metric_id=metric_id,
                                                      metric_instance=metric_instance,
                                                      mparams_api=mparams_api,
                                                      mparams_by_pos_api_list=mparams_by_pos_api_list,
                                                      scale_stat_depth=scale_stat_depth,
                                                      numeric_values_only=comp_ctx.numeric_values_only)
        except Exception as exc:
            # MSTAND-1339: do not log errors for one metric twice
            if metric_id not in comp_ctx.error_metrics:
                logging.error("DETAILED METRIC ERROR [id=%s]: %s, details: %s", metric_id, exc, traceback.format_exc())
            if not comp_ctx.metric_ctx.global_ctx.skip_metric_errors:
                raise
            metric_vals_api = SerpMetricValuesForAPI(total_value=None, has_details=False, has_error=True,
                                                     error_message=str(exc))
    else:
        try:
            metric_vals_api = compute_metric_querywise(comp_ctx=comp_ctx,
                                                       metric_id=metric_id,
                                                       metric_instance=metric_instance,
                                                       mparams_api=mparams_api,
                                                       numeric_values_only=comp_ctx.numeric_values_only)
        except Exception as exc:
            if metric_id not in comp_ctx.error_metrics:
                # MSTAND-1339: do not log errors for one metric twice
                logging.error("QUERYWISE METRIC ERROR [id=%s]: %s, details: %s", metric_id, exc, traceback.format_exc())
            if not comp_ctx.metric_ctx.global_ctx.skip_metric_errors:
                raise
            metric_vals_api = SerpMetricValuesForAPI(total_value=None, has_details=False, has_error=True, error_message=str(exc))

    return metric_vals_api


def compute_metric_querywise(comp_ctx, metric_id, metric_instance, mparams_api, numeric_values_only=False):
    """
    :type comp_ctx: OfflineComputationCtx
    :type metric_id: int
    :type metric_instance:
    :type mparams_api: SerpMetricParamsForAPI
    :type numeric_values_only: bool
    :rtype: SerpMetricValuesForAPI
    """
    is_local_calc_mode = comp_ctx.metric_ctx.global_ctx.is_local_calc_mode()

    start_time = time.time() if is_local_calc_mode else None
    raw_metric_value = metric_instance.value(mparams_api)
    if is_local_calc_mode:
        comp_ctx.accumulate_metric_calc_time(metric_id, start_time)

    prepared_metric_val = postprocess_metric_value(raw_metric_value, numeric_values_only)

    metric_vals_api = SerpMetricValuesForAPI(total_value=prepared_metric_val, has_details=False)

    if mparams_api.collect_scale_stats:
        for result_api in mparams_api.results:
            metric_vals_api.update_scale_stats(result_api.scale_stats)
        metric_vals_api.update_serp_data_stats(mparams_api.serp_data_stats)

    return metric_vals_api


def postprocess_metric_value(raw_metric_value, numeric_values_only):
    """
    :type raw_metric_value:
    :type numeric_values_only: bool
    :return:
    """
    prepared_metric_val = prepare_raw_metric_value(raw_metric_value)

    if numeric_values_only:
        validate_metric_value(prepared_metric_val)
        prepared_metric_val = convert_bool_to_number(prepared_metric_val)
    return prepared_metric_val


def convert_bool_to_number(raw_metric_value):
    if raw_metric_value is True:
        return 1
    elif raw_metric_value is False:
        return 0

    return raw_metric_value


def compute_metric_detailed(comp_ctx, metric_id, metric_instance, mparams_api, mparams_by_pos_api_list, scale_stat_depth,
                            numeric_values_only):
    """
    :type comp_ctx: OfflineComputationCtx
    :type metric_id: int
    :type metric_instance:
    :type mparams_api: SerpMetricParamsForAPI
    :type mparams_by_pos_api_list: list[SerpMetricParamsByPositionForAPI]
    :type scale_stat_depth:
    :type numeric_values_only: bool
    :rtype: SerpMetricValuesForAPI
    """
    metric_vals_api = SerpMetricValuesForAPI(total_value=None, has_details=True)

    is_local_calc_mode = comp_ctx.metric_ctx.global_ctx.is_local_calc_mode()

    if om_caps.has_serp_depth(metric_instance):
        # should be fast, don't measure time here
        serp_depth = metric_instance.serp_depth()
    else:
        serp_depth = None

    for rindex, mparams_by_pos_api in enumerate(mparams_by_pos_api_list):
        if serp_depth is not None:
            if rindex >= serp_depth:
                break

        # scale stat depth is individual for metric
        mparams_by_pos_api.result.init_scale_stat_depth(scale_stat_depth)

        start_time = time.time() if is_local_calc_mode else None
        raw_metric_value = metric_instance.value_by_position(mparams_by_pos_api)
        if is_local_calc_mode:
            comp_ctx.accumulate_metric_calc_time(metric_id, start_time)

        prepared_metric_val = postprocess_metric_value(raw_metric_value, numeric_values_only)

        if mparams_api.collect_scale_stats:
            metric_vals_api.update_serp_data_stats(mparams_by_pos_api.serp_data_stats)

        metric_val_by_pos_api = SerpMetricValueByPositionForAPI(value=prepared_metric_val, index=rindex)
        metric_vals_api.add_position_value(metric_val_by_pos_api)

        if mparams_api.collect_scale_stats:
            metric_vals_api.update_scale_stats(mparams_by_pos_api.result.scale_stats)
            metric_vals_api.update_serp_data_stats(mparams_by_pos_api.serp_data_stats)

    agg_params_api = SerpMetricAggregationParamsForAPI(mparams_api=mparams_api, metric_values=metric_vals_api)

    agg_start_time = time.time() if is_local_calc_mode else None
    raw_total_metric_value = metric_instance.aggregate_by_position(agg_params_api)
    if is_local_calc_mode:
        comp_ctx.accumulate_metric_calc_time(metric_id, agg_start_time)

    prepared_total_metric_val = postprocess_metric_value(raw_total_metric_value, numeric_values_only)
    metric_vals_api.total_value = prepared_total_metric_val

    if mparams_api.collect_scale_stats:
        metric_vals_api.update_serp_data_stats(agg_params_api.serp_data_stats)
    return metric_vals_api


def validate_metric_value(raw_metric_value):
    if raw_metric_value is None:
        return

    # if isinstance(raw_metric_value, bool):
    #     raise Exception("Metric value is not numeric/null, value type is: {}".format(type(raw_metric_value)))

    if not isinstance(raw_metric_value, Real):
        raise Exception("Metric value is not numeric/null, value type is: {}".format(type(raw_metric_value)))

    if math.isnan(raw_metric_value) or math.isinf(raw_metric_value):
        raise Exception("Metric value is NaN or Inf")


def prepare_raw_metric_value(raw_metric_val):
    if isinstance(raw_metric_val, (types.GeneratorType, list, tuple)):
        metric_vals = list(raw_metric_val)
        if len(metric_vals) == 1:
            return metric_vals[0]
        else:
            return metric_vals
    else:
        return raw_metric_val


def _normalize_metric_value(value):
    if isinstance(value, list):
        return value
    return [value]


def dump_metric_values_internal(comp_ctx, serpset_metric_vals):
    """
    :type comp_ctx: OfflineComputationCtx
    :type serpset_metric_vals: SerpsetMetricValues
    :rtype:
    """
    start_time = time.time()

    serpset_id = comp_ctx.serpset_id
    metric_ctx = comp_ctx.metric_ctx
    metric_storage = metric_ctx.metric_storage

    for metric_id, metric_key in usix.iteritems(metric_ctx.plugin_container.plugin_key_map):

        metric_has_data = True
        metric_file = metric_storage.metric_by_serpset(metric_id, serpset_id)

        if metric_id in comp_ctx.error_metrics:
            logging.warning("Metric %s (id %s) has errors on serpset %s", metric_key, metric_id, serpset_id)
            metric_has_data = False
        elif metric_id not in serpset_metric_vals.metric_vals:
            logging.warning("Metric %s (id %s) has no values on serpset %s", metric_key, metric_id, serpset_id)
            metric_has_data = False

        if not metric_has_data:
            logging.warning("Writing empty metric results for metric %s to %s", metric_key, metric_file)
            ufile.write_text_file(metric_file, "")
            continue

        querywise_metric_values = serpset_metric_vals.metric_vals[metric_id]

        temp_metric_file = metric_file + ".tmp"
        with ufile.fopen_write(temp_metric_file) as metric_fd:
            for qid, raw_value in usix.iteritems(querywise_metric_values):
                tsv_row = [qid] + _normalize_metric_value(raw_value)
                utsv.dump_row_to_fd(tsv_row, metric_fd)
        os.rename(temp_metric_file, metric_file)
        logging.debug("Written metric results to %s", metric_file)

    umisc.log_elapsed(start_time, "Internal metric results dump completed for serpset %s", serpset_id)


def upload_one_serpset_to_yt(comp_ctx, serpset_table, yt_api):
    """
    :type comp_ctx: OfflineComputationCtx
    :type serpset_table: str
    :type yt_api: YtApi
    :rtype: None
    """
    assert serpset_table
    met_ctx = comp_ctx.metric_ctx
    serp_storage = met_ctx.parsed_serp_storage

    queries_file = serp_storage.queries_by_serpset(comp_ctx.serpset_id)
    markup_file = serp_storage.markup_by_serpset(comp_ctx.serpset_id)

    if comp_ctx.load_urls:
        urls_file = serp_storage.urls_by_serpset(comp_ctx.serpset_id)
    else:
        urls_file = None

    serpset_file_handler = SerpSetFileHandler(queries_file=queries_file, markup_file=markup_file,
                                              urls_file=urls_file, mode="read")

    with serpset_file_handler:
        serpset_reader = SerpSetReader.from_file_handler(serpset_file_handler)

        upload_one_serpset_to_yt_impl(comp_ctx=comp_ctx, serpset_reader=serpset_reader,
                                      yt_api=yt_api, serpset_yt_table=serpset_table)


def upload_one_serpset_to_yt_impl(comp_ctx, serpset_reader, serpset_yt_table, yt_api):
    """
    :type comp_ctx: OfflineComputationCtx
    :type serpset_reader: SerpSetReader
    :type serpset_yt_table: str
    :type yt_api: YtApi
    :rtype:
    """
    logging.info("start upload serpset %s", comp_ctx.serpset_id)
    start_time = time.time()
    # append to store all computation serpsets in one table
    serpset_yt_table_path = YtApi.TablePath(serpset_yt_table, append=True)

    row_buf = []
    for index, parsed_serp in enumerate(serpset_reader):
        serpset_row = {
            "serpset-id": comp_ctx.serpset_id,
            "query-info": parsed_serp.query_info.serialize(),
            "markup-info": parsed_serp.markup_info.serialize(),
            "urls-info": parsed_serp.urls_info.serialize(),
        }
        row_buf.append(serpset_row)
        if len(row_buf) > OfflineDefaultValues.SERPSET_UPLOAD_BUFFER_SIZE:
            yt_api.write_table(path=serpset_yt_table_path, rows=row_buf)
            row_buf = []
    if row_buf:
        yt_api.write_table(path=serpset_yt_table_path, rows=row_buf)

    umisc.log_elapsed(start_time, "serpset %s uploaded", comp_ctx.serpset_id)


class OfflineMetricYtCalculator:
    def __init__(self, metric_ctx):
        """
        :type metric_ctx: OfflineMetricCtx
        """
        self.metric_ctx = metric_ctx

    def __call__(self, row):
        """
        :type row: dict
        :rtype: dict
        """
        serpset_id = row["serpset-id"]
        query_info_data = row["query-info"]
        query_info = SerpQueryInfo.deserialize(query_info_data)
        mc_query = query_info.query_key.serialize_external()

        if query_info.is_failed_serp and self.metric_ctx.skip_failed_serps:
            failed_serp_row = {
                ExtMetricResultsTable.Fields.SERPSET_ID: serpset_id,
                ExtMetricResultsTable.Fields.MSTAND_QID: query_info.qid,
                ExtMetricResultsTable.Fields.MSTAND_METRIC_ID: None,
                ExtMetricResultsTable.Fields.QUERY: mc_query,
                ExtMetricResultsTable.Fields.METRIC: None,
                ExtMetricResultsTable.Fields.TOTAL_VALUE: None,
                ExtMetricResultsTable.Fields.VALUES_BY_POSITION: None,
                ExtMetricResultsTable.Fields.HAS_ERROR: True,
                ExtMetricResultsTable.Fields.ERROR_MESSAGE: None
            }
            yield failed_serp_row
            return

        markup_info_data = row["markup-info"]
        markup_info = SerpMarkupInfo.deserialize(markup_info_data)
        urls_info_data = row["urls-info"]
        urls_info = SerpUrlsInfo.deserialize(urls_info_data)

        parsed_serp = ParsedSerp(query_info=query_info, markup_info=markup_info, urls_info=urls_info)

        # FIXME (proper computation context for pool)
        exp = Experiment(serpset_id=serpset_id)
        obs = Observation(obs_id=None, control=exp, dates=None)
        comp_ctx = OfflineComputationCtx(metric_ctx=self.metric_ctx, experiment=exp, observation=obs)

        serp_metric_vals_iter = compute_metrics_on_serp(comp_ctx=comp_ctx, parsed_serp=parsed_serp)

        # serialization is similar to ExtMetricResultsWriter._make_query_result()

        metric_keys = self.metric_ctx.plugin_container.plugin_key_map

        serp_result_count = markup_info.result_count()
        serp_metric_vals = list(serp_metric_vals_iter)
        if not serp_metric_vals:
            # MSTAND-1892: add one fake row
            smv = self.make_fake_value(qid=query_info.qid)
            fake_metric_row = self.make_one_metric_row(serp_metric_val=smv, mc_query=mc_query,
                                                       metric_keys={}, serp_result_count=serp_result_count,
                                                       serpset_id=serpset_id)
            yield fake_metric_row

        else:
            for smv in serp_metric_vals:
                one_metric_row = self.make_one_metric_row(serp_metric_val=smv, mc_query=mc_query, metric_keys=metric_keys,
                                                          serp_result_count=serp_result_count, serpset_id=serpset_id)

                yield one_metric_row

    @classmethod
    def make_fake_value(cls, qid):
        one_metric_val = SerpMetricValuesForAPI(total_value=None, values_by_position=None,
                                                has_error=True, error_message="No valid metric values in this computation")
        return SerpMetricValue(qid=qid, metric_id=None, one_metric_val=one_metric_val)

    def make_one_metric_row(self, serp_metric_val, mc_query, metric_keys, serp_result_count, serpset_id):
        """
        :type serp_metric_val: SerpMetricValue
        :type mc_query: dict
        :type metric_keys: dict
        :type serp_result_count: int
        :type serpset_id: str
        :rtype: dict
        """
        ext_metric_val = ExtMetricValue(serp_metric_val.one_metric_val)

        if metric_keys:
            metric_alias = metric_keys[serp_metric_val.metric_id].str_key()
        else:
            metric_alias = None

        good_alias_prefix = self.metric_ctx.global_ctx.mc_alias_prefix
        # error_alias_prefix = self.metric_ctx.global_ctx.mc_error_alias_prefix

        if metric_alias:
            good_metric_key = "{}{}".format(good_alias_prefix, metric_alias)
        else:
            good_metric_key = None

        # error_metric_key = "{}{}".format(error_alias_prefix, metric_alias)

        values_by_position = ext_metric_val.pos_values
        if values_by_position is not None:
            values_count = len(values_by_position)
            # MSTAND-1769: make all values-by-position same size
            if values_count < serp_result_count:
                values_by_position += [None] * (serp_result_count - values_count)

        one_metric_row = {
            ExtMetricResultsTable.Fields.SERPSET_ID: serpset_id,
            ExtMetricResultsTable.Fields.MSTAND_QID: serp_metric_val.qid,
            ExtMetricResultsTable.Fields.MSTAND_METRIC_ID: serp_metric_val.metric_id,
            ExtMetricResultsTable.Fields.QUERY: mc_query,
            ExtMetricResultsTable.Fields.METRIC: good_metric_key,
            ExtMetricResultsTable.Fields.TOTAL_VALUE: ext_metric_val.total_value,
            ExtMetricResultsTable.Fields.VALUES_BY_POSITION: values_by_position,
            ExtMetricResultsTable.Fields.HAS_ERROR: ext_metric_val.has_error,
            ExtMetricResultsTable.Fields.ERROR_MESSAGE: ext_metric_val.error_message
        }
        return one_metric_row
