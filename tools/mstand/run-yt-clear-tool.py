#!/usr/bin/env python3

import argparse
import logging

from typing import Optional

import cli_tools.yt_clear_tool as yt_clear_tool
import mstand_utils.args_helpers as mstand_uargs
import pytlib.client_yt as ytcli
import yaqutils.file_helpers as ufile
import yaqutils.misc_helpers as umisc
import yt.wrapper as yt

from mstand_enums.mstand_general_enums import YtResourceTypeEnum

DEFAULT_POOLS_PATH = "../../quality/mstand_metrics/online_production/pools"
UNTAR_DESTINATION_POOLS_PATH = "./pools"


def get_percent(percent_as_str: str) -> float:
    try:
        percent = float(percent_as_str)
        assert 0 <= percent <= 100
    except (ValueError, AssertionError):
        raise Exception("Percent value must be a float or integer number in the range of 0-100.")
    return percent


def check_usage_limit_is_not_exceeded(yt_resource_type: str,
                                      yt_client: yt.YtClient,
                                      usage_percent_limit: Optional[float] = None) -> bool:
    if usage_percent_limit is None:
        return True

    usage = yt.get_attribute(path="//sys/accounts/mstand",
                             attribute="resource_usage/{}".format(yt_resource_type),
                             client=yt_client)
    usage_limit = yt.get_attribute(path="//sys/accounts/mstand",
                                   attribute="resource_limits/{}".format(yt_resource_type),
                                   client=yt_client)

    usage_percent = usage / usage_limit * 100
    if usage_percent >= usage_percent_limit:
        logging.info("{} percent limit is exceeded: {:.2f}% >= {:.2f}%".format(yt_resource_type, usage_percent,
                                                                               usage_percent_limit))
        return False
    else:
        logging.info("{} percent limit is not exceeded: {:.2f}% < {:.2f}%".format(yt_resource_type, usage_percent,
                                                                                  usage_percent_limit))
        return True


def run_clearing(extended: bool,
                 yt_client: yt.YtClient,
                 ab_token_file: str,
                 disk_usage_percent_limit: Optional[float] = None,
                 used_chunks_percent_limit: Optional[float] = None,
                 used_nodes_percent_limit: Optional[float] = None,
                 pools_path: Optional[str] = None) -> None:
    if disk_usage_percent_limit or used_chunks_percent_limit or used_nodes_percent_limit:
        disk_usage_is_ok = check_usage_limit_is_not_exceeded(yt_resource_type=YtResourceTypeEnum.DISK_SPACE,
                                                             yt_client=yt_client,
                                                             usage_percent_limit=disk_usage_percent_limit)
        chunks_count_is_ok = check_usage_limit_is_not_exceeded(yt_resource_type=YtResourceTypeEnum.CHUNK_COUNT,
                                                               yt_client=yt_client,
                                                               usage_percent_limit=used_chunks_percent_limit)
        nodes_count_is_ok = check_usage_limit_is_not_exceeded(yt_resource_type=YtResourceTypeEnum.NODE_COUNT,
                                                              yt_client=yt_client,
                                                              usage_percent_limit=used_nodes_percent_limit)

        if disk_usage_is_ok and chunks_count_is_ok and nodes_count_is_ok:
            logging.info("Quota limits are not exceeded.")
            return

    logging.info("Clearing tmp paths...")
    yt_clear_tool.run_clearing(path_to_clear="//home/mstand",
                               yt_client=yt_client,
                               access_time_limit=yt_clear_tool.get_timeout_limit_datetime("30"),
                               substring_blacklist=["/squeeze/"],
                               substring="/tmp/",
                               remove_really=True,
                               verbose=True)

    logging.info("Clearing nirvana data...")
    yt_clear_tool.run_clearing(path_to_clear="//home/mstand",
                               yt_client=yt_client,
                               access_time_limit=yt_clear_tool.get_timeout_limit_datetime("30"),
                               substring_blacklist=["/squeeze/"],
                               substring="/nirvana/",
                               remove_really=True,
                               verbose=True)

    logging.info("Clearing robots squeezes...")
    yt_clear_tool.run_clearing(path_to_clear="//home/mstand/squeeze",
                               yt_client=yt_client,
                               access_time_limit=yt_clear_tool.get_timeout_limit_datetime("30"),
                               owners=["robot-mstand", "robot-toloka"],
                               substring_blacklist=["/surveys_strongest_testids/", "/surveys_yuid_testids/"],
                               remove_really=True,
                               verbose=True)
    
    logging.info("Clearing mstand_metrics fast...")
    yt_clear_tool.run_clearing(path_to_clear="//home/mstand/mstand_metrics/fast",
                               yt_client=yt_client,
                               access_time_limit=yt_clear_tool.get_timeout_limit_datetime("0"),
                               creation_time_limit=yt_clear_tool.get_timeout_limit_datetime("14"),
                               remove_really=True ,
                               verbose=True)

    if extended:
        logging.info("Getting blacklist of tables required for surplus validation squeezes...")

        assert pools_path, "pools path must be specified for extended clearing"

        tables_blacklist = yt_clear_tool.get_tables_blacklist_by_pools(
            pools_path=pools_path,
            services=["web-auto", "web-auto-extended"],
            ab_token_file=ab_token_file
        )

        logging.info("Clearing squeezes including web...")
        yt_clear_tool.run_clearing(path_to_clear="//home/mstand/squeeze",
                                   yt_client=yt_client,
                                   access_time_limit=yt_clear_tool.get_timeout_limit_datetime("90"),
                                   tables_blacklist=tables_blacklist,
                                   substring_blacklist=["/zen/", "/zen-surveys/", "/surveys_strongest_testids/",
                                                        "/surveys_yuid_testids/"],
                                   remove_really=True,
                                   verbose=True)
    else:
        logging.info("Clearing squeezes except web...")
        yt_clear_tool.run_clearing(path_to_clear="//home/mstand/squeeze",
                                   yt_client=yt_client,
                                   access_time_limit=yt_clear_tool.get_timeout_limit_datetime("90"),
                                   substring_blacklist=["/web/", "/touch/", "/web-desktop-extended/",
                                                        "/web-touch-extended/"],
                                   remove_really=True,
                                   verbose=True)

    logging.info("Clearing has completed")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--extended",
        required=False,
        action="store_true",
        help="Clear all squeezes including web"
    )
    parser.add_argument(
        "--disk-usage-percent-limit",
        help="Acceptable upper percentage limit of the disk usage",
        type=get_percent
    )
    parser.add_argument(
        "--used-chunks-percent-limit",
        help="Acceptable upper percentage limit of the used chunks",
        type=get_percent
    )
    parser.add_argument(
        "--used-nodes-percent-limit",
        help="Acceptable upper percentage limit of the used nodes",
        type=get_percent
    )

    mstand_uargs.add_yt_auth_token_file(parser=parser)
    mstand_uargs.add_ab_token_file(parser=parser)

    pools_group = parser.add_mutually_exclusive_group(required=False)
    pools_group.add_argument(
        "--pools-archive-path",
        help="Path to the archive with pools to ignore in the case of the extended clearing mode",
    )
    pools_group.add_argument(
        "--pools-path",
        help="Path to pools to ignore in the case of the extended clearing mode",
    )

    mstand_uargs.add_yt_server(parser=parser)

    return parser.parse_args()


def main() -> None:
    umisc.configure_logger()

    cli_args = parse_args()
    if cli_args.pools_archive_path:
        pools_path = UNTAR_DESTINATION_POOLS_PATH
        ufile.make_dirs(directory=pools_path)
        ufile.untar_directory(tar_name=cli_args.pools_archive_path, dest_dir=pools_path)
    else:
        pools_path = cli_args.pools_path or DEFAULT_POOLS_PATH

    ytcli.set_yt_token_to_env_from_file(yt_token_file=cli_args.yt_auth_token_file)
    yt_client = ytcli.create_client(server=cli_args.server, be_verbose=False)
    run_clearing(extended=cli_args.extended,
                 yt_client=yt_client,
                 disk_usage_percent_limit=cli_args.disk_usage_percent_limit,
                 used_chunks_percent_limit=cli_args.used_chunks_percent_limit,
                 used_nodes_percent_limit=cli_args.used_nodes_percent_limit,
                 pools_path=pools_path,
                 ab_token_file=cli_args.ab_token_file)


if __name__ == "__main__":
    main()
