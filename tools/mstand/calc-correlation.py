#!/usr/bin/env python3

import argparse

import adminka.ab_cache

import experiment_pool.pool_helpers as pool_helpers
import reports.compute_correlation as comp_corr
import mstand_utils.args_helpers as mstand_uargs
import mstand_utils.mstand_module_helpers as mstand_umodule
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc

from reports import CorrOutContext
from reports import CorrCalcContext

from user_plugins import PluginKey


def parse_args():
    parser = argparse.ArgumentParser(description="Compute metric results correlation")
    uargs.add_verbosity(parser)

    uargs.add_input(parser, help_message="pool(s) with metric results", multiple=True)
    uargs.add_output(parser)

    mstand_uargs.add_modules(parser, default_class="CorrelationPearson", default_module="correlations")
    mstand_uargs.add_output_pool(parser, help_message="write merged pool to this file")
    mstand_uargs.add_save_to_dir(parser)
    mstand_uargs.add_ab_token_file(parser)

    parser.add_argument("--criteria-left")
    parser.add_argument("--criteria-right")
    parser.add_argument(
        "--main-metric",
        help="Compare this metric against others"
    )

    mstand_uargs.add_output_tsv(parser, help_message="write results as TSV to this file (use with --main-metric)")
    mstand_uargs.add_min_pvalue(parser)

    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)

    criteria_left_name = cli_args.criteria_left
    criteria_right_name = cli_args.criteria_right

    correlation = mstand_umodule.create_user_object(module_name=cli_args.module_name,
                                                    class_name=cli_args.class_name,
                                                    source=cli_args.source,
                                                    kwargs=cli_args.user_kwargs)

    merged_pool = pool_helpers.load_and_merge_pools(cli_args.input_file)

    if cli_args.output_pool:
        pool_helpers.dump_pool(merged_pool, cli_args.output_pool)

    main_metric_alias = cli_args.main_metric

    criteria_pair = comp_corr.make_criteria_pair(criteria_left_name, criteria_right_name)

    main_metric_key = PluginKey(name=main_metric_alias) if main_metric_alias else None

    adminka_session = adminka.ab_cache.AdminkaCachedApi(auth_token=mstand_uargs.get_token(cli_args.ab_token_file))

    corr_out_ctx = CorrOutContext(output_file=cli_args.output_file,
                                  output_tsv=cli_args.output_tsv,
                                  save_to_dir=cli_args.save_to_dir)

    corr_calc_ctx = CorrCalcContext(criteria_pair=criteria_pair,
                                    main_metric_key=main_metric_key,
                                    min_pvalue=cli_args.min_pvalue)

    comp_corr.calc_correlation_main(correlation=correlation,
                                    pool=merged_pool,
                                    corr_calc_ctx=corr_calc_ctx,
                                    corr_out_ctx=corr_out_ctx,
                                    adminka_session=adminka_session)


if __name__ == "__main__":
    main()
