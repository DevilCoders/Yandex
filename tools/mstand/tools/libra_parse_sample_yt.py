#!/usr/bin/env python2
# -*- coding: utf-8 -*-

import argparse
import logging

# noinspection PyPackageRequirements
import yt.wrapper as yt


def module_filter(module):
    module_name = getattr(module, "__name__", "")

    if ".yson" in module_name or "yson." in module_name:
        return True

    if module_name in ("hashlib", "_hashlib", "hmac"):
        return False

    module_file = getattr(module, "__file__", "")
    if not module_file:
        return False
    if module_file.endswith(".so"):
        return False

    return True


def prepare_suggest_data(item):
    suggest = item.GetSuggest()
    if not suggest:
        return None
    return {
        "TimeSinceFirstChange": suggest.TimeSinceFirstChange,
        "TimeSinceLastChange": suggest.TimeSinceLastChange,
        "UserKeyPressesCount": suggest.UserKeyPressesCount,
        "UserInput": suggest.UserInput,
        "TpahLog": suggest.TpahLog,
    }


def example_reduce_suggest(keys, rows):
    logging.error("yuid: %s", keys["key"])
    # noinspection PyUnresolvedReferences,PyPackageRequirements
    import libra
    for request in libra.ParseSession(rows, "blockstat.dict"):
        logging.error("reqid: %s", request.ReqId)
        logging.error("timestamp: %s", request.Timestamp)
        logging.error("suggest: %s", prepare_suggest_data(request))


def example_reduce_news(keys, rows):
    logging.error("yuid: %s", keys["key"])
    rows = list(rows)
    for row in rows:
        logging.error("%s %s", row["subkey"], row["value"])

    # noinspection PyUnresolvedReferences,PyPackageRequirements
    import libra
    for request in libra.ParseSession(rows, "blockstat.dict"):
        if request.IsA("TNewsRequestProperties"):
            logging.error("timestamp: %s", request.Timestamp)
            logging.error("reqid: %s", request.ReqId)
            logging.error("news: %s", )
            logging.error("main blocks: %s", len(request.GetMainBlocks()))
            logging.error("parallel blocks: %s", len(request.GetParallelBlocks()))


def example_reduce_watchlog(keys, rows):
    logging.error("yuid: %s", keys["key"])
    # noinspection PyUnresolvedReferences,PyPackageRequirements
    import libra
    container = libra.Parse(rows, "blockstat.dict")
    logging.error("GetRequests: %s", container.GetRequests())
    logging.error("GetUserActions: %s", container.GetUserActions())
    logging.error("GetSessionYandexTechEvents: %s", container.GetSessionYandexTechEvents())
    logging.error("GetOwnEvents: %s", container.GetOwnEvents())
    logging.error("GetPageViews: %s", container.GetPageViews())
    logging.error("GetMailEvents: %s", container.GetMailEvents())


REDUCERS = {
    "suggest": example_reduce_suggest,
    "news": example_reduce_news,
    "watchlog": example_reduce_watchlog,
}

SESSION_TYPES = {
    "suggest": "search",
    "news": "search",
    "watchlog": "watch_log_tskv",
}


def parse_args(parser):
    parser.add_argument(
        "--mode",
        required=True,
        help="test mode",
        choices=sorted(REDUCERS.keys()),
    )
    parser.add_argument(
        "--day",
        required=True,
        help="User session day (format YYYY-MM-DD)",
    )
    parser.add_argument(
        "--yuid",
        required=False,
        help="yuid",
    )
    return parser.parse_args()


def main():
    parser = argparse.ArgumentParser(description="Parse one user session for one day")
    cli_args = parse_args(parser)
    logging.basicConfig(format="[%(levelname)-7s] %(asctime)s - %(message)s", level=logging.INFO)

    # noinspection PyUnresolvedReferences
    yt_config = yt.config
    """:type: dict"""
    yt_config["proxy"]["url"] = "hahn"
    yt_config["pickling"]["module_filter"] = module_filter
    yt_config["pickling"]["force_using_py_instead_of_pyc"] = True

    reducer = REDUCERS[cli_args.mode]
    session_type = SESSION_TYPES[cli_args.mode]
    day = cli_args.day
    yuid = cli_args.yuid

    if len(day) != 10:
        raise Exception("Seems that date format is incorrect (expected YYYY-MM-DD).")

    table = "//user_sessions/pub/{}/daily/{}/clean".format(session_type, day)
    source = yt.TablePath(table, exact_key=yuid)

    logging.info("source table: %s", source)

    with yt.TempTable() as tmp:
        yt.run_reduce(
            reducer,
            source_table=source,
            destination_table=tmp,
            local_files=["libra.so", "blockstat.dict"],
            reduce_by=["key"],
            sort_by=["key", "subkey"],
            spec={"reducer": {"enable_input_table_index": True}},
            format=yt.YsonFormat(control_attributes_mode="row_fields"),
        )


if __name__ == "__main__":
    main()
