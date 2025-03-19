import argparse
import importlib
import logging
import sys


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--from-date", help="from date")
    parser.add_argument("--to-date", help="to date")
    parser.add_argument("--plot-file-path", help="plot file path", default="plot.png")
    parser.add_argument("--proxy", help="YT proxy", default="hahn")
    parser.add_argument("--pool", help="YT pool", default=None)
    parser.add_argument(
        "--ignore-cache",
        help="ignore cache",
        default=False,
        action="store_true",
    )
    parser.add_argument(
        "--only-nighttime",
        help="process only nighttime escalations",
        default=False,
        action="store_true",
    )
    parser.add_argument(
        "-v", "--verbose",
        help="verbose mode",
        default=0,
        action="count",
    )
    args = parser.parse_args()

    log_level = max(0, logging.ERROR - 10 * int(args.verbose))
    logger = logging.getLogger()
    logger.setLevel(log_level)

    formatter = logging.Formatter("[%(levelname)s] [%(asctime)s] %(message)s")

    ch = logging.StreamHandler(stream=sys.stderr)
    ch.setLevel(log_level)
    ch.setFormatter(formatter)
    logger.addHandler(ch)

    module = importlib.import_module(
        "cloud.blockstore.tools.analytics.escalation-stats.lib"
    )
    module.analyze_escalation_freq(
        args.from_date,
        args.to_date,
        args.plot_file_path,
        args.only_nighttime,
        args.proxy,
        args.pool,
        args.ignore_cache,
        logger,
    )
