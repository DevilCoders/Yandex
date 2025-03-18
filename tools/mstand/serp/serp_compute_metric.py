import logging
import os
import time
import traceback
from typing import List

import experiment_pool.pool_helpers as phelp
import pytlib.yt_io_helpers as yt_io
import pytlib.yt_misc_helpers as yt_misc
import serp.serp_compute_metric_single as scm_single
import yaqutils.file_helpers as ufile
import yaqutils.misc_helpers as umisc
import yaqutils.nirvana_helpers as unirv
import yaqutils.six_helpers as usix
import yaqutils.time_helpers as utime

import yt.wrapper.common as yt_common

from experiment_pool import MetricResult
from experiment_pool import MetricType
from experiment_pool import MetricValues  # noqa
from experiment_pool import Pool  # noqa
from mstand_enums.mstand_offline_enums import OfflineCalcMode
from mstand_utils import OfflineDefaultValues
from pytlib import YtApi
from serp import ExtMetricResultsTable
from serp import MetricDataStorage  # noqa
from serp import OfflineComputationCtx
from serp import OfflineGlobalCtx  # noqa
from serp import OfflineMetricCtx  # noqa
from yaqlibenums import YtOperationTypeEnum
from yaqutils import MRTable


def fill_pool_results(pool, metric_ctx, pool_results):
    """
    :type pool: Pool
    :type metric_ctx: OfflineMetricCtx
    :type pool_results: dict[str, dict[int, MetricValues]]
    :rtype:
    """
    logging.info("Filling pool results")
    for obs in pool.observations:
        for exp in obs.all_experiments():
            if should_skip_experiment(exp, obs):
                continue
            exp_results = pool_results[exp.serpset_id]
            for metric_id, metric_values in usix.iteritems(exp_results):
                metric_instance = metric_ctx.plugin_container.plugin_instances[metric_id]
                metric_key = metric_ctx.plugin_container.plugin_key_map[metric_id]
                coloring = metric_instance.coloring

                metric_result = MetricResult(metric_key=metric_key,
                                             metric_values=metric_values,
                                             metric_type=MetricType.OFFLINE,
                                             coloring=coloring)
                exp.add_metric_result(metric_result)


def dump_result_pool(pool, metric_storage):
    phelp.dump_pool(pool, metric_storage.pool_path())

    # actually, pool was already dumped to cache.
    # but this is mstand's common metric result output format.
    metric_result_pool_path = os.path.join(metric_storage.metric_dir(), "pool.json")
    phelp.dump_pool(pool, metric_result_pool_path)


def pack_calculation_results(metric_storage, metric_tar_name):
    """
    :type metric_storage: MetricDataStorage
    :type metric_tar_name: str
    :rtype: None
    """
    logging.info("Creating tar archive %s with metric results", metric_tar_name)
    ufile.tar_directory(path_to_pack=metric_storage.metric_dir(), tar_name=metric_tar_name,
                        dir_content_only=True, verbose=False)
    logging.info("Creating metric results tar archive %s done", metric_tar_name)


def create_external_output(metric_ctx, output_file):
    """
    :type metric_ctx: OfflineMetricCtx
    :type output_file: str
    :rtype:
    """
    if metric_ctx.global_ctx.is_yt_calc_mode():
        logging.info("We're in YT calc mode, skipping .tgz output")
        return

    path_to_pack = metric_ctx.metric_storage.metric_dir_external()
    if metric_ctx.global_ctx.gzip_external_output:
        # no need to pack twice gzipped jsons (even without compression)
        compressor = ufile.Compressor.NONE
    else:
        compressor = ufile.Compressor.GZIP

    ufile.tar_directory(path_to_pack=path_to_pack, tar_name=output_file, compressor=compressor, dir_content_only=True)


def create_external_output_as_mr_table(global_ctx, output_file):
    """
    :type global_ctx: OfflineGlobalCtx
    :type output_file: str
    :rtype:
    """
    if global_ctx.is_local_calc_mode():
        logging.info("We're in local calc mode, skipping MR output")
        return

    assert global_ctx.metric_results_table, "metric results table should be not-empty in YT calc mode"
    mr_table = MRTable(table_path=global_ctx.metric_results_table, yt_cluster=global_ctx.yt_cluster)
    mr_table.dump_to_file(output_file)


def estimate_max_job_delay(pool, max_processes):
    """
    :type pool: Pool
    :type max_processes: int
    :rtype:
    """
    jobs_total = len(pool.all_serpset_ids())
    processes = umisc.get_optimal_process_number(jobs_total, max_processes)
    if processes == 1:
        return 0.0

    return float(processes) / 4.0


def should_skip_experiment(exp, obs):
    """
    :type exp: Experiment
    :type obs: Observation
    :rtype: bool
    """
    if not exp.serpset_id:
        logging.info("Skipping experiment %s in observation %s (no serpset id).", exp, obs)
        return True
    if exp.has_errors():
        logging.info("Skipping experiment %s in observation %s (errors in experiment).", exp, obs)
        return True
    return False


def calc_offline_metric_local(pool, metric_ctx, max_processes):
    """
    :type pool: Pool
    :type metric_ctx: OfflineMetricCtx
    :type max_processes: int | None
    """
    obs_count = len(pool.observations)
    serpset_ids = pool.all_serpset_ids()
    logging.info("Computing metric on pool of %d observations (%d serpsets)", obs_count, len(serpset_ids))

    arg_set = set()

    # each process is delayed for random value limited by max_delay.
    # it's used to spread data flushes for different serpsets.
    max_delay = estimate_max_job_delay(pool, max_processes)
    logging.info("Max job delay is: %.2f seconds", max_delay)

    for obs in pool.observations:
        for exp in obs.all_experiments():
            if should_skip_experiment(exp, obs):
                continue
            comp_ctx = OfflineComputationCtx(metric_ctx, observation=obs, experiment=exp)
            arg_set.add(comp_ctx)

    args = sorted(arg_set)
    logging.info("Running metric computation for %d jobs", len(args))
    # use large chunks to reduce contexts copying in multiprocessing

    if not args:
        logging.info("All metric results are taken from cache, nothing to calculate.")
        return

    pool_results = {}
    compute_result = umisc.par_imap_unordered(mp_compute_metric, args, max_processes, chunksize=None)
    # res is a aggregated metric result on serpset
    for pos, (serpset_id, res) in enumerate(compute_result):
        if isinstance(res, BaseException):
            logging.error("Exception in par_imap_unordered for serpset %s: %s", serpset_id, res)
            raise res
        umisc.log_progress("offline metric calculation", pos, len(args))
        unirv.log_nirvana_progress("offline metric calculation", pos, len(args))
        pool_results[serpset_id] = res

    fill_pool_results(pool, metric_ctx, pool_results)
    logging.info("Metric computation done.")


def mp_compute_metric(comp_ctx):
    """
    :type comp_ctx: OfflineComputationCtx
    :rtype: tuple[str, dict[int, MetricValues]]
    """
    try:
        logging.info("Computing metrics for serpset %s", comp_ctx.serpset_id)

        computation_start_time = time.time()
        comp_ctx.reset_metric_calc_times()

        if comp_ctx.is_serpset_computed():
            logging.info("Metric results for %s taken from cache.", comp_ctx)
        else:
            scm_single.compute_metric_by_serpset(comp_ctx)

        res = scm_single.aggregate_metric_by_serpset(comp_ctx)
        comp_ctx.log_metric_calc_times()
        umisc.log_elapsed(computation_start_time, "pure serpset %s computation", comp_ctx.serpset_id)
        return comp_ctx.serpset_id, res
    except BaseException as exc:
        logging.info("Exception in mp_compute_metric serpset %s: %s, details: %s",
                     comp_ctx.serpset_id, exc, traceback.format_exc())
        return comp_ctx.serpset_id, exc


def autodetect_calc_mode(metric_ctx):
    """
    :param metric_ctx: OfflineMetricCtx
    :rtype:
    """
    if metric_ctx.global_ctx.is_auto_calc_mode():
        if metric_ctx.plugin_container.size() > OfflineDefaultValues.LOCAL_MODE_MAX_METRICS:
            metric_ctx.global_ctx.calc_mode = OfflineCalcMode.YT
        else:
            metric_ctx.global_ctx.calc_mode = OfflineCalcMode.LOCAL
        logging.info("autodetected calc mode: %s", metric_ctx.global_ctx.calc_mode)
    else:
        logging.info("calc mode is forced to be %s", metric_ctx.global_ctx.calc_mode)


def calc_offline_metric_main(pool, metric_ctx, threads, yt_auth_token_file=None):
    """
    :type pool: Pool
    :type metric_ctx: OfflineMetricCtx
    :type threads: int
    :type yt_auth_token_file: str | None
    :rtype: None
    """
    autodetect_calc_mode(metric_ctx)

    if metric_ctx.global_ctx.is_yt_calc_mode():
        calc_offline_metric_yt(pool=pool, metric_ctx=metric_ctx, yt_auth_token_file=yt_auth_token_file)
    else:
        calc_offline_metric_local(pool=pool, metric_ctx=metric_ctx, max_processes=threads)
    # cache's internal pool output
    dump_result_pool(pool, metric_ctx.metric_storage)

    pool.save_metric_stats()
    pool.log_metric_stats()


def select_yt_cluster(global_ctx):
    """
    :type global_ctx: OfflineGlobalCtx
    :rtype:
    """
    logging.info("selecting YT cluster for computations")
    if global_ctx.yt_cluster:
        logging.info("yt cluster parameter is implicitly specified: %s", global_ctx.yt_cluster)
    else:
        logging.info("yt cluster parameter is not specified, autodetecting.")
        # MSTAND-1789: take same cluster as in mstand's nirvana block
        env_yt_cluster = os.environ.get("YT_CLUSTER_NAME")
        if env_yt_cluster:
            logging.info("environment YT cluster: %s", env_yt_cluster)
            global_ctx.yt_cluster = env_yt_cluster
        else:
            logging.warning("environment YT cluster is empty, using default")
            global_ctx.yt_cluster = OfflineDefaultValues.DEF_YT_CLUSTER

    assert global_ctx.yt_cluster
    logging.info("finally, yt cluster is %s", global_ctx.yt_cluster)


def calc_offline_metric_yt(pool, metric_ctx, yt_auth_token_file):
    """
    :type pool: Pool
    :type metric_ctx: OfflineMetricCtx
    :type yt_auth_token_file: str | None
    :rtype: None
    """
    if yt_auth_token_file:
        yt_misc.prepare_yt_env_from_files(yt_token_file=yt_auth_token_file)
    else:
        logging.warning("YT token file is not specified. Assuming this is a command-line manual run and YT_TOKEN is set")

    select_yt_cluster(metric_ctx.global_ctx)

    obs_count = len(pool.observations)
    serpset_ids = pool.all_serpset_ids()
    logging.info("Computing metric on pool of %d observations (%d serpsets)", obs_count, len(serpset_ids))

    computations_set = set()

    for obs in pool.observations:
        for exp in obs.all_experiments():
            if should_skip_experiment(exp, obs):
                continue
            comp_ctx = OfflineComputationCtx(metric_ctx, observation=obs, experiment=exp)
            computations_set.add(comp_ctx)

    computations = sorted(computations_set)
    logging.info("Running YT metric computation for %d unique args (serpsets)", len(computations))

    metric_results_table = compute_metrics_on_yt(computations)
    metric_ctx.global_ctx.metric_results_table = metric_results_table
    logging.info("NOTE: metric results aggregation is DISABLED in YT calculation mode")
    logging.info("Metric computation done.")


def prepare_yt_environment(yt_api, global_ctx):
    logging.info("creating temp and output folders")
    yt_api.create_folder(path=global_ctx.yt_temp_dir, ignore_existing=True)
    yt_api.create_folder(path=global_ctx.yt_output_dir, ignore_existing=True)


def compute_metrics_on_yt(computations):
    """
    :type computations: list[OfflineComputationCtx]
    :rtype: str
    """
    if not computations:
        logging.warning("computation list is empty, doing nothing")
        return

    metric_ctx = computations[0].metric_ctx
    global_ctx = metric_ctx.global_ctx

    yt_api = YtApi.construct_with_yt_cluster(yt_cluster=global_ctx.yt_cluster)

    prepare_yt_environment(yt_api=yt_api, global_ctx=global_ctx)

    results_ttl = global_ctx.output_yt_table_ttl
    logging.info("creating temp table, ttl = %s seconds", results_ttl)

    time_label = utime.timestamp_to_str_msk_nospaces(time.time())
    results_prefix = "mstand_offline_metric_results_{}_".format(time_label)
    output_table_path = yt_api.create_temp_table(path=global_ctx.yt_output_dir, prefix=results_prefix,
                                                 expiration_timeout_sec=results_ttl)

    metric_results_table_schema = ExtMetricResultsTable.get_offline_metric_yt_output_table_schema()
    metric_results_table = YtApi.TablePath(output_table_path, schema=metric_results_table_schema)

    add_output_table_env_info(output_table=metric_results_table, yt_api=yt_api, global_ctx=global_ctx)

    logging.info("output table path: %s", metric_results_table)

    serpsets_prefix = "mstand_offline_serpsets_{}_".format(time_label)

    with yt_api.temp_table(path=global_ctx.yt_temp_dir, prefix=serpsets_prefix) as serpsets_table:
        upload_parsed_serpsets_to_yt(computations, serpsets_table=serpsets_table, yt_api=yt_api)
        compute_metrics_on_serpsets_table(serpsets_table=serpsets_table, metrics_results_table=metric_results_table,
                                          yt_api=yt_api, metric_ctx=metric_ctx)

    logging.info("metric computation completed, results stored in table %s within %s seconds", metric_results_table, results_ttl)
    metric_results_table_url = yt_api.make_yt_table_url(path=metric_results_table)
    logging.info("metric results YT table url: %s", metric_results_table_url)
    return metric_results_table


def upload_parsed_serpsets_to_yt(computations: List[OfflineComputationCtx],
                                 serpsets_table: str, yt_api: YtApi):
    # TODO (happy future): this upload should be done by Metrics export
    start_time = time.time()
    logging.info("uploading serpsets to table %s", serpsets_table)
    for comp_ctx in computations:
        scm_single.upload_one_serpset_to_yt(comp_ctx, yt_api=yt_api, serpset_table=serpsets_table)

    umisc.log_elapsed(start_time, "all serpsets uploaded to yt")


def add_output_table_env_info(output_table: str, yt_api: YtApi, global_ctx: OfflineGlobalCtx):
    mstand_env_attr = {
        "mstand-workflow-id": global_ctx.env_workflow_id,
        "mstand-workflow-url": unirv.make_nirvana_workflow_url_by_id(global_ctx.env_workflow_id)
    }
    yt_api.set_attribute(path=output_table, attribute="mstand-env", value=mstand_env_attr)


def get_offline_metric_yt_operation_acl() -> list:
    operation_acl = {
        "permissions": ["read", "manage"],
        "subjects": OfflineDefaultValues.OPERATION_OWNERS,
        "action": "allow"
    }
    return [operation_acl]


def compute_metrics_on_serpsets_table(serpsets_table: str, metrics_results_table: str,
                                      yt_api: YtApi, metric_ctx: OfflineMetricCtx):
    logging.info("running metrics computation on yt")
    start_time = time.time()
    mapper = scm_single.OfflineMetricYtCalculator(metric_ctx=metric_ctx)

    input_size = yt_api.row_count(serpsets_table)
    metric_number = metric_ctx.plugin_container.size()

    # one job size is an empirical constant to make optimal job duration about 1-2 minutes.
    one_job_size = 20000

    job_count = int(input_size * metric_number / one_job_size)
    logging.info("desired job count: %s", job_count)
    job_count = max(job_count, OfflineDefaultValues.MIN_YT_JOB_COUNT)

    yt_pool = metric_ctx.global_ctx.yt_pool

    logging.info("estimated job count: %s, yt pool: %s", job_count, yt_pool)
    if not yt_pool:
        logging.warning("YT pool is not specified. Your calculation may be slower.")

    operation_acl = get_offline_metric_yt_operation_acl()
    operation_spec = yt_io.get_yt_operation_spec(max_failed_job_count=10,
                                                 operation_executor_types=YtOperationTypeEnum.MAP,
                                                 yt_pool=yt_pool,
                                                 job_count=job_count,
                                                 use_porto_layer=True,
                                                 acl=operation_acl,
                                                 memory_limit=1024 * yt_common.MB)

    yt_api.run_map(mapper, src_path=serpsets_table,
                   dst_path=metrics_results_table, spec=operation_spec)

    umisc.log_elapsed(start_time, "yt metric computation completed")

    sort_operation_spec = yt_io.get_yt_operation_spec(max_failed_job_count=10,
                                                      yt_pool=yt_pool,
                                                      use_porto_layer=False,
                                                      acl=operation_acl,
                                                      memory_limit=1024 * yt_common.MB)

    yt_api.run_sort(src_path=metrics_results_table, dst_path=metrics_results_table,
                    sort_by=[ExtMetricResultsTable.Fields.SERPSET_ID], spec=sort_operation_spec)

    umisc.log_elapsed(start_time, "yt metric results sorting completed")
