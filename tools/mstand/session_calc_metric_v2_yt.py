#!/usr/bin/env python2.7
# -*- coding: utf-8 -*-

import argparse
import logging
import os
import shutil

import adminka.filter_fetcher
import experiment_pool.pool_helpers as pool_helpers
import mstand_utils.args_helpers as mstand_uargs
import mstand_utils.mstand_module_helpers as mstand_umodule
import session_metric.metric_v2
import yaqutils.args_helpers as uargs
import yaqutils.file_helpers as ufile
import yaqutils.misc_helpers as umisc
from experiment_pool import MetricColoring
from user_plugins import PluginKey


def parse_args():
    parser = argparse.ArgumentParser(description="Calculate metric on MR")
    mstand_uargs.add_common_session_metric_yt_args(parser)
    uargs.add_input(parser, help_message="read metric data from this file")
    uargs.add_output(parser)
    uargs.add_threads(parser)
    parser.add_argument(
        "--save-to-dir",
        type=str,
        help="save metric results to dir",
    )
    parser.add_argument(
        "--save-to-tar",
        type=str,
        help="save metric results to tar",
    )
    parser.add_argument(
        "--load-from-dir",
        type=str,
        help="load metric results from dir",
    )
    parser.add_argument(
        "--load-from-tar",
        type=str,
        help="load metric results from tar",
    )
    parser.add_argument(
        "--learning-algorithm",
        type=str,
        default="gbdt",
        help="algorithm to predict metric values",
    )
    parser.add_argument(
        "--learning-trees",
        type=int,
        default=100,
        help="Number of trees in the learning algorithm"
    )
    parser.add_argument(
        "--learning-rate",
        type=float,
        default=0.05,
        help="Learning rate in the learning algorithm"
    )
    parser.add_argument(
        "--learning-nodes",
        type=int,
        default=30,
        help="Number of nodes in the tree in gbdt algorithm"
    )
    parser.add_argument(
        "--learning-depth",
        type=int,
        default=6,
        help="Tree depth in matrixnet algorithm"
    )
    return parser.parse_args()


def get_yt_client(cli_args):
    import pytlib.client_yt
    return pytlib.client_yt.create_client(
        server=cli_args.server,
        verbose_operations=cli_args.verbose,
        quiet=cli_args.quiet,
        filter_so=cli_args.filter_so,
    )


def get_metric(cli_args):
    metric = mstand_umodule.create_user_object(
        kwargs=cli_args.user_kwargs,
        module_name=cli_args.module_name,
        class_name=cli_args.class_name,
        source=cli_args.source,
    )
    MetricColoring.set_metric_instance_coloring(metric, cli_args.set_coloring)
    return metric


def get_metric_key(cli_args):
    return PluginKey.generate(cli_args.module_name, cli_args.class_name, cli_args.set_alias)


def get_learning_params(cli_args):
    return {
        "learning_trees": cli_args.learning_trees,
        "learning_rate": cli_args.learning_rate,
        "learning_nodes": cli_args.learning_nodes,
        "learning_depth": cli_args.learning_depth
    }


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)

    load = cli_args.load_from_dir or cli_args.load_from_tar
    save = cli_args.save_to_dir or cli_args.save_to_tar

    if save:
        import session_yt.metric_v2_yt
        # noinspection PyPackageRequirements
        import yt.wrapper as yt
        pool = pool_helpers.load_pool(cli_args.input_file)
        yt_client = get_yt_client(cli_args)
        with yt.Transaction(client=yt_client) as transaction:
            session_yt.metric_v2_yt.calc_mr_for_pool(
                pool=pool,
                metric=get_metric(cli_args),
                metric_key=get_metric_key(cli_args),
                squeeze_path=cli_args.squeeze_path,
                services=cli_args.services,
                threads=cli_args.threads,
                client=yt_client,
                transaction=transaction,
                save_to_dir=cli_args.save_to_dir,
                save_to_tar=cli_args.save_to_tar,
                yt_pool=cli_args.yt_pool,
            )
    elif load:
        base_dir_is_tmp = False
        if cli_args.load_from_dir:
            base_dir = cli_args.load_from_dir
        else:
            base_dir = ufile.extract_tar_to_temp(cli_args.load_from_tar)
            base_dir_is_tmp = True
        pool = pool_helpers.load_pool(os.path.join(base_dir, "pool.json"))
        adminka.filter_fetcher.fetch_all(pool, allow_bad_filters=True)
        session_metric.metric_v2.calc_metric_for_pool_local(
            pool=pool,
            base_dir=base_dir,
            algorithm=cli_args.learning_algorithm,
            algorithm_params=get_learning_params(cli_args)
        )
        if base_dir_is_tmp:
            logging.info("removing tmp dir: %s", base_dir)
            shutil.rmtree(base_dir)
    else:
        import session_yt.metric_v2_yt
        # noinspection PyPackageRequirements
        import yt.wrapper as yt
        yt_client = get_yt_client(cli_args)
        pool = pool_helpers.load_pool(cli_args.input_file)
        with yt.Transaction(client=yt_client) as transaction:
            mr_result = session_yt.metric_v2_yt.calc_mr_for_pool(
                pool=pool,
                metric=get_metric(cli_args),
                metric_key=get_metric_key(cli_args),
                squeeze_path=cli_args.squeeze_path,
                services=cli_args.services,
                threads=cli_args.threads,
                client=yt_client,
                transaction=transaction,
                save_to_dir=None,
                save_to_tar=None,
                yt_pool=cli_args.yt_pool,
            )
            session_yt.metric_v2_yt.calc_metric_for_pool(
                pool=pool,
                mr_result=mr_result,
                algorithm=cli_args.learning_algorithm,
                algorithm_params=get_learning_params(cli_args),
                client=yt_client
            )

    logging.info("Dumping logs to " + cli_args.output_file)
    pool_helpers.dump_pool(pool, cli_args.output_file)


if __name__ == "__main__":
    main()
