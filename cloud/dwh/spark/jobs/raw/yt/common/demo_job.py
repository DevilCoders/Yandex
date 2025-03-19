import argparse

from spark.jobs.common import get_logger
import vault_client

from spark.jobs.config import DWH_SECRET_ID


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--secret", help="yav oauth token", required=True)
    return parser.parse_args()


def main():
    logger = get_logger()
    args = parse_args()
    logger.info("Starting.")
    yav = vault_client.instances.Production(authorization=args.secret)
    secret = yav.get_version(DWH_SECRET_ID)['value']
    logger.info(secret["dwh-etl-user-login"])
    logger.info("Done.")


if __name__ == '__main__':
    main()
