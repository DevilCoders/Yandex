#!/usr/bin/env python2
# -*- coding: utf-8 -*-

import argparse
import logging
import os.path
import time

import yaqutils.args_helpers as uargs
import yaqutils.file_helpers as ufile
import yaqutils.misc_helpers as umisc
import yaqutils.nirvana_helpers as unirv


example_text = '''example:

 python nirvana-progress-checker.py --steps 5 --delay 600'''


def parse_args():
    parser = argparse.ArgumentParser(
        description="Test nirvana progress log [MSTAND-1390]",
        epilog=example_text,
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    uargs.add_verbosity(parser)
    parser.add_argument(
        "--steps",
        type=int,
        default=1,
        help="How many steps it needs to do (default=1)",
    )
    parser.add_argument(
        "--delay",
        type=int,
        default=60,
        help="Delay between steps in seconds (default=60)",
    )

    return parser.parse_args()


def get_status_log():
    status_log = unirv.get_nirvana_status_log()
    if status_log:
        if os.path.exists(status_log):
            text = ufile.read_text_file(status_log)
            logging.info("File %s contains:\n%s", status_log, text)
        else:
            logging.warning("Status log file %s not exists", status_log)
    else:
        logging.warning("Status log file name not found")


def run_test(steps, delay):
    logging.info("Run test with params: steps=%d, delay=%d", steps, delay)

    for i in range(steps):
        logging.info("Step %i from %i", i + 1, steps)
        time.sleep(delay)
        unirv.log_nirvana_progress("test", i, steps)
        get_status_log()

    time.sleep(delay)


def main():
    start = time.time()

    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)

    run_test(cli_args.steps, cli_args.delay)

    logging.info("Total process time: %.2f sec.", time.time() - start)


if __name__ == "__main__":
    main()
