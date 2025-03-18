#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import logging
import os
import shutil

import adminka.ab_cache
import experiment_pool.pool_helpers as pool_helpers
import postprocessing.compute_criteria as comp_crit
import postprocessing.criteria_utils as crit_utils
import mstand_utils.args_helpers as mstand_uargs
import mstand_utils.mstand_module_helpers as mstand_umodule
import yaqutils.args_helpers as uargs
import yaqutils.file_helpers as ufile
import yaqutils.misc_helpers as umisc
from user_plugins import PluginKey


def parse_args():
    parser = argparse.ArgumentParser(description="Calculate stat criteria")
    uargs.add_verbosity(parser)
    uargs.add_input(parser, help_message="tarball or directory with input data")
    uargs.add_output(parser)
    mstand_uargs.add_modules(parser, default_class="TTest", default_module="criterias")
    uargs.add_threads(parser, default=None)
    parser.add_argument(
        "--synthetic",
        type=int,
        help="synthetic controls number",
    )
    mstand_uargs.add_output_html_vertical(parser)
    mstand_uargs.add_ab_token_file(parser)
    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)

    criteria = mstand_umodule.create_user_object(module_name=cli_args.module_name,
                                                 class_name=cli_args.class_name,
                                                 source=cli_args.source,
                                                 kwargs=cli_args.user_kwargs)

    crit_utils.validate_criteria(criteria)

    if os.path.isdir(cli_args.input_file):
        base_dir = cli_args.input_file
        base_dir_is_tmp = False
    else:
        base_dir = ufile.extract_tar_to_temp(cli_args.input_file)
        base_dir_is_tmp = True

    pool = pool_helpers.load_pool(os.path.join(base_dir, "pool.json"))

    data_type = crit_utils.get_pool_data_type(pool)

    if cli_args.synthetic:
        pool = comp_crit.build_synthetic_pool(pool, data_type)

    criteria_key = PluginKey.generate(cli_args.module_name, cli_args.class_name)

    pool = comp_crit.calc_criteria(criteria=criteria, criteria_key=criteria_key, pool=pool,
                                   base_dir=base_dir, data_type=data_type,
                                   synthetic=cli_args.synthetic, threads=cli_args.threads)

    if cli_args.synthetic:
        pool.synthetic_summaries = comp_crit.calc_synthetic_summaries(pool.observations, data_type)

    pool_helpers.dump_pool(pool, cli_args.output_file)

    if base_dir_is_tmp:
        logging.info("removing tmp dir: %s", base_dir)
        shutil.rmtree(base_dir)

    # for easier summary viewing in Nirvana
    if cli_args.output_html_vertical:
        session = adminka.ab_cache.AdminkaCachedApi(auth_token=mstand_uargs.get_token(cli_args.ab_token_file))
        comp_crit.save_summary(pool, cli_args.output_html_vertical, session)


if __name__ == "__main__":
    main()
