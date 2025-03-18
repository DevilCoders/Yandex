from __future__ import print_function
import argparse

from antirobot.tools.daily_routine.lib import stream_converters
from nile.api.v1 import clusters


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('--proxy', default="hahn", help="YT cluster")

    parser.add_argument('--daemon', help="antrobot-daemon-log")
    parser.add_argument('--event', help="antrobot-binary-event-log")
    parser.add_argument('--watch', help="bs-watch-log")
    parser.add_argument('--fraud-uids', help="fraud_uids")
    parser.add_argument('--usclean', help="usersessions/clean")
    parser.add_argument('--usfrauds', help="usersessions/frauds")

    parser.add_argument('--output-unite', help="unite table")
    parser.add_argument('--output-event', help="event table")
    parser.add_argument('--output-factors', help="factors table")
    parser.add_argument('--output-wyuid', help="watchlog yandexuid table")
    parser.add_argument('--output-wips', help="watchlog ips table")
    parser.add_argument('--output-us', help="usersessions table")

    return parser.parse_args()


def main():
    args = parse_args()

    env = {
        # "templates": {"tmp_root": "//home/antirobot/tmp"},
        "remove_strategy": {
            "remote_tmp_files": "on_job_finish",
            "remote_tmp_tables": "on_job_finish",
        },
        "parallel_operations_limit": 5,
    }
    cluster = clusters.Hahn()
    cluster = cluster.env(**env)

    stream_converters.Combo().run(
        cluster,
        [args.event,
         args.daemon,
         args.watch,
         args.fraud_uids,
         args.usfrauds,
         args.usclean],
        [args.output_unite,
         args.output_event,
         args.output_factors,
         args.output_wyuid,
         args.output_wips,
         args.output_us],
    )


if __name__ == "__main__":
    main()
