#!/usr/bin/env python3.6
# -*- coding: utf-8 -*-
import argparse
import logging

import adminka.ab_cache
import adminka.pool_validation
import experiment_pool.pool_helpers as phelp
import mstand_utils.args_helpers as mstand_uargs
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc
import reports.metrics_compare as rmc


def parse_args():
    parser = argparse.ArgumentParser(description="Compare metrics results")
    uargs.add_verbosity(parser)
    uargs.add_input(parser, help_message="pool(s) with metric results", multiple=True)
    mstand_uargs.add_threshold(parser)
    mstand_uargs.add_output_tsv(parser)
    mstand_uargs.add_output_wiki(parser)
    mstand_uargs.add_output_wiki_vertical(parser)
    mstand_uargs.add_output_html_vertical(parser)
    mstand_uargs.add_output_pool(parser, help_message="write merged pool to this file")
    mstand_uargs.add_ab_token_file(parser)

    uargs.add_boolean_argument(
        parser,
        name="show-all-gray",
        help_message="show all-gray experiments in output tables",
        default=True
    )
    uargs.add_boolean_argument(
        parser,
        name="show-same-color",
        help_message="show same-color experiments in output tables",
        default=True
    )
    uargs.add_boolean_argument(
        parser,
        name="validate-pool",
        help_message="validate pool",
        default=True
    )
    parser.add_argument(
        "--source-url",
        help="display operation URL at the beginning of wiki/html page",
        default=""
    )
    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)

    outputs = [
        cli_args.output_tsv,
        cli_args.output_wiki,
        cli_args.output_wiki_vertical,
        cli_args.output_html_vertical,
    ]
    if not any(outputs):
        raise Exception("Set at least one of: --output-tsv/wiki/wiki-vertical/html-vertical")

    pool = phelp.load_and_merge_pools(cli_args.input_file)

    if cli_args.output_pool:
        phelp.dump_pool(pool, cli_args.output_pool)

    # rhelp.set_metric_colors(pool, cli_args.threshold)

    session = adminka.ab_cache.AdminkaCachedApi(auth_token=mstand_uargs.get_token(cli_args.ab_token_file))
    session.preload_testids(pool.all_testids())

    if cli_args.validate_pool:
        validation_result = adminka.pool_validation.validate_pool(pool, session)
        logging.info("Validation result:\n%s", validation_result.pretty_print())
    else:
        validation_result = None

    logging.info("Creating template env")
    env = rmc.create_template_environment(pool=pool, session=session, validation_result=validation_result,
                                          threshold=cli_args.threshold, show_all_gray=cli_args.show_all_gray,
                                          show_same_color=cli_args.show_same_color, source_url=cli_args.source_url)

    logging.info("Rendering templates")

    wiki_text = env.get_template("wiki_metrics.tpl").render()
    wiki_text_vertical = env.get_template("wiki_metrics_vertical.tpl").render()
    html_text_vertical = env.get_template("html_metrics_vertical.tpl").render()

    tsv_fd = rmc.get_fd(cli_args.output_tsv)
    if tsv_fd:
        rmc.write_tsv(tsv_fd, pool, session, cli_args.threshold)

    rmc.write_markup(cli_args.output_html_vertical, html_text_vertical)
    rmc.write_markup(cli_args.output_wiki, wiki_text)
    rmc.write_markup(cli_args.output_wiki_vertical, wiki_text_vertical)


if __name__ == "__main__":
    main()
