import argparse
from pathlib import Path
import yaml

from .utils.Platform import Platform
from .utils.log import setup_logger
from .commands import project_check


def get_defaults(path):
    f = Path(path)
    with f.open() as fojb:
        data = yaml.safe_load(fojb.read())
    return data


def parse_args():
    parser = argparse.ArgumentParser(description="This tools checks qloud apps and their configuration status",
                                     usage="\n./platform-balancer-info -p kinopoisk -e production -a auth\n"
                                           "'production' is environment in qloud terms\n"
                                           "'auth' is an application name",
                                     formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument("-t", "--token", help="Platform OAuth token. Default from $PLATFORM_TOKEN env variable")
    parser.add_argument("-d", "--debug", help="Print debug log", action="store_true")
    parser.add_argument("--defaults", help="Recommended config values", default="./best_practice.yaml")
    parser.add_argument("-p", "--project", help="Platform project", required=True)
    parser.add_argument("-e", "--environment", help="Project environment")
    parser.add_argument("-a", "--application", help="Project application")

    return parser.parse_args()


def main():
    args = parse_args()
    setup_logger(args.debug)

    pl = Platform(project=args.project, token=args.token)

    defaults = get_defaults(args.defaults)
    project_check(pl, defaults, args.environment, args.application)


if __name__ == '__main__':
    main()
