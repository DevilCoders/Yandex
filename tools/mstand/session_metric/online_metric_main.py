# coding=utf-8
import mstand_metric_helpers.common_metric_helpers as mhelp
import mstand_utils.args_helpers as mstand_uargs
import session_metric.lamps_helpers as ulamps

from experiment_pool import Pool  # noqa
from experiment_pool import pool_helpers as phelp
from session_metric import MetricRunner


def calc_online_metric_main(cli_args, calc_backend, pool):
    """
    :type cli_args: argparse.Namespace
    :type calc_backend: session_local.metric_local.MetricBackendLocal | session_yt.metric_yt.MetricBackendYT
    :type pool: Pool
    :rtype: None
    """
    import adminka.ab_cache  # MSTAND-1978 otherwise fails on YT
    session = adminka.ab_cache.AdminkaCachedApi(auth_token=mstand_uargs.get_token(cli_args.ab_token_file))
    pool.clear_metric_results()

    metric_container = mhelp.create_container_from_cli_args(cli_args)
    metric_runner = MetricRunner.from_cli_args(
        cli_args=cli_args,
        metric_container=metric_container,
        calc_backend=calc_backend,
    )

    import adminka.ab_helpers  # MSTAND-1978 otherwise fails on YT
    if not cli_args.all_users:
        adminka.ab_helpers.validate_and_enrich(
            pool,
            session=session,
            add_filters=cli_args.use_filters,
            services=cli_args.services,
            ignore_triggered_testids_filter=cli_args.ignore_triggered_testids_filter,
            allow_fake_services=True,
            allow_bad_filters=True,
        )
    else:
        pool = phelp.create_all_users_mode_pool(pool)

    metric_runner.calc_for_pool(
        pool=pool,
        save_to_dir=cli_args.save_to_dir,
        save_to_tar=cli_args.save_to_tar,
        threads=cli_args.threads,
        batch_min=cli_args.experiment_batch_min,
        batch_max=cli_args.experiment_batch_max,
    )

    if not cli_args.lamps_mode:
        phelp.dump_pool(pool, cli_args.output_file)
    else:
        result = ulamps.calc_lamps(calc_backend, cli_args, pool)
        phelp.dump_pool(result, cli_args.output_file)
