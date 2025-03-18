#!/usr/bin/env python3
import argparse

import postprocessing.postproc_engine as pp_engine
import postprocessing.postproc_helpers as pp_helpers
import mstand_utils.args_helpers as mstand_uargs
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc
from experiment_pool import pool_helpers
from experiment_pool.filter_metric import MetricFilter


def parse_args():
    parser = argparse.ArgumentParser(description="mstand buckets")
    uargs.add_verbosity(parser)
    uargs.add_threads(parser, default=None)

    uargs.add_input(parser, help_message="input TAR file or directory with metric results")
    uargs.add_output(parser, help_message="write output pool to this file")

    mstand_uargs.add_save_to_dir(parser)
    mstand_uargs.add_save_to_tar(parser)

    mstand_uargs.add_postprocess_props(parser)
    mstand_uargs.add_plugin_params(parser)

    uargs.add_boolean_argument(parser, "parse-kwargs-json", "parse kwargs as JSON", default=True)

    uargs.add_boolean_argument(parser,
                               name="preserve-original-names",
                               help_message="preserve metric names, do not add suffix with postrpocess name",
                               default=False)

    MetricFilter.add_cli_args(parser)

    return parser.parse_args()


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)

    if not cli_args.save_to_tar and not cli_args.save_to_dir:
        raise Exception("Output not specified. Please specify --save-to-dir or --save-to-tar parameter.")

    pp_container = pp_helpers.create_postprocessor_from_cli(cli_args)

    metrics_filter = MetricFilter.from_cli_args(cli_args)

    pool = pp_engine.postprocess_and_repack(
        input_path=cli_args.input_file,
        dest_dir=cli_args.save_to_dir,
        dest_tar=cli_args.save_to_tar,
        pp_container=pp_container,
        threads=cli_args.threads,
        metrics_filter=metrics_filter,
        preserve_original_names=cli_args.preserve_original_names,
    )

    pool_helpers.dump_pool(pool, cli_args.output_file)


if __name__ == "__main__":
    main()
