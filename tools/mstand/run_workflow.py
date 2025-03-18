#!/usr/bin/env python3

"""
This piece of code was taken from
arcadia/quality/ab_testing/scripts/exp_veles/exp_mr_server/scripts/calc_mstand_executable.py
during MSTAND-1518 and rewritten.
"""
import logging

import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc
import yaqutils.json_helpers as ujson
import yaqutils.six_helpers as usix
import mstand_utils.args_helpers as mstand_uargs
from argparse import ArgumentParser
from nirvana_api import WorkflowRunner
from yaqutils import Period

MSTAND_DEFAULT_POOL_PLACEHOLDER = "mstand_default"
MSTAND_REGULAR_POOL = "mstand_regular"
MSTAND_DEFAULT_GODA_POOL_PLACEHOLDER = "goda"
MSTAND_REGULAR_GODA_POOL = "goda_robot-mstand"


def parse_args():
    parser = ArgumentParser(description="Launch mstand calculation")
    uargs.add_input(parser, help_message="path to json with options")
    uargs.add_output(parser, help_message="result of calculation")
    uargs.add_verbosity(parser)
    mstand_uargs.add_token(parser, name="nirvana", help_message="Nirvana OAuth token")
    mstand_uargs.add_token(parser, name="yt", default="mstand_yt_hahn_token", help_message="YT OAuth token")
    mstand_uargs.add_mode(parser, choices=("mstand"), default="mstand")
    # ttl in arguments is in minutes
    mstand_uargs.add_ttl(parser)
    mstand_uargs.add_additional_ttl(parser)

    return parser.parse_args()


def encode_struct(value):
    if isinstance(value, str):
        return value.encode("utf-8")
    if isinstance(value, dict):
        return {encode_struct(k): encode_struct(v) for k, v in usix.iteritems(value)}
    return value


def get_run_params(cli_args):
    raw_options_data = ujson.load_from_file(cli_args.input_file)

    options_data = encode_struct(raw_options_data)
    cfg_data = options_data.get("config") or {}

    notes = cfg_data.get("notes") or {}
    metric_params = cfg_data["metric_params"]
    user = cfg_data["user"]

    yt_pool = metric_params.get("yt_pool") or MSTAND_DEFAULT_POOL_PLACEHOLDER
    if yt_pool == MSTAND_DEFAULT_POOL_PLACEHOLDER:
        yt_pool = MSTAND_REGULAR_POOL
    elif yt_pool == MSTAND_DEFAULT_GODA_POOL_PLACEHOLDER:
        yt_pool = MSTAND_REGULAR_GODA_POOL
    metric_params["yt_pool"] = yt_pool
    metric_params["yt_token"] = cli_args.yt_token

    metric_params.setdefault("owners", []).append(user)
    metric_params.setdefault("instance", "")

    return dict(
        task=options_data["task"],
        obsid=int(cfg_data["obsid"]),
        metric_params=metric_params,
        start=options_data["start_date"],
        finish=options_data["end_date"],
        user=user,
        ab_url=notes.get("url"),
    )


def main():
    cli_args = parse_args()
    assert cli_args.ttl > cli_args.additional_ttl, "job timeout should be greater, than calculation timeout"
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)
    params = get_run_params(cli_args)

    logging.info("Starting calculation for user %s, using observation %s with dates %s-%s",
                 params["user"], params["obsid"], params["start"], params["finish"])

    workflow_runner = WorkflowRunner(token=cli_args.nirvana_token,
                                     workflow_id=params["metric_params"]["workflow"],
                                     instance_id=params["metric_params"]["instance"],
                                     params=params,
                                     mode=cli_args.mode,
                                     calc_timeout=Period.MINUTE * (cli_args.ttl - cli_args.additional_ttl))

    try:
        result = workflow_runner.run()
    except Exception as e:
        logging.error("Execution failed: %s", str(e), exc_info=True)
        workflow_runner.stop()
        raise

    ujson.dump_to_file(result, cli_args.output_file)

    logging.info("Calculation was successfully completed")


if __name__ == "__main__":
    main()
