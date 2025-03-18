# -*- coding: utf-8 -*-
import datetime
import functools
import logging
import multiprocessing

# noinspection PyPackageRequirements
import yt.wrapper as yt
# noinspection PyPackageRequirements
import yt.wrapper.common as yt_common

import yaqutils.time_helpers as utime


@yt.aggregator
def map_sessions_yuids(rows):
    yuids = set()
    for row in rows:
        yuid = row["key"]
        yuids.add(yuid)
    for yuid in yuids:
        yield {"yuid": yuid}


def reduce_squeeze_yuids(keys, _):
    yuid = keys["yuid"]
    yield {"yuid": yuid}


def create_client():
    config = {
        "proxy": {
            "url": "hahn",
        },
    }
    return yt.YtClient(config=config)


def mp_save_yuids_from_sessions(day, filename):
    file_path = "{}.{}".format(filename, utime.format_date(day))
    yt_path = "//user_sessions/pub/search/daily/{}/clean".format(utime.format_date(day, pretty=True))
    yt_client = create_client()
    yt_table = yt.TablePath(yt_path, lower_key="y", upper_key="z", client=yt_client)
    yt_spec = {
        "max_failed_job_count": 10,
        "data_size_per_job": yt_common.GB,
        "mapper": {"enable_input_table_index": True},
    }
    with yt.TempTable(client=yt_client) as tmp:
        yt.run_map(
            map_sessions_yuids,
            source_table=yt_table,
            destination_table=tmp,
            spec=yt_spec,
            client=yt_client,
            format=yt.YsonFormat(control_attributes_mode="row_fields"),
        )
        logging.info("download %s to %s", yt_path, file_path)
        with open(file_path, "w") as f:
            for row in yt.read_table(tmp, client=yt_client):
                f.write(row["yuid"])
                f.write("\n")


def mp_save_yuids_from_squeezes(day, testid, filename, history=None):
    file_path = "{}.{}".format(filename, utime.format_date(day))
    if history:
        yt_path = "//home/mstand/squeeze/history/web/{}/{}_{}/{}".format(testid,
                                                                         utime.format_date(history.start),
                                                                         utime.format_date(history.end),
                                                                         utime.format_date(day))
    else:
        yt_path = "//home/mstand/squeeze/testids/web/{}/{}".format(testid, utime.format_date(day))
    yt_client = create_client()
    yt_spec = {
        "max_failed_job_count": 10,
        "data_size_per_job": yt_common.MB * 300,
        "reducer": {"enable_input_table_index": True},
    }
    with yt.TempTable(client=yt_client) as tmp:
        yt.run_reduce(
            reduce_squeeze_yuids,
            source_table=yt_path,
            destination_table=tmp,
            reduce_by=["yuid"],
            spec=yt_spec,
            client=yt_client,
            format=yt.YsonFormat(control_attributes_mode="row_fields"),
        )
        logging.info("download %s to %s", yt_path, file_path)
        with open(file_path, "w") as f:
            for row in yt.read_table(tmp, client=yt_client):
                f.write(row["yuid"])
                f.write("\n")


def mp_find_yuids_in_sessions(day, yuids):
    assert yuids
    assert isinstance(yuids, set)
    yt_path = "//user_sessions/pub/search/daily/{}/clean".format(utime.format_date(day, pretty=True))
    logging.info("check %s", yt_path)
    yt_client = create_client()
    file_path = "yuids.found.{}".format(utime.format_date(day))
    lower_key = min(yuids)
    upper_key = max(yuids) + "z"
    found = set()
    yt_table = yt.TablePath(yt_path, lower_key=lower_key, upper_key=upper_key, columns=["key"], client=yt_client)
    with open(file_path, "w") as f:
        for row in yt.read_table(yt_table, client=yt_client):
            yuid = row["key"]
            if yuid not in found and yuid in yuids:
                found.add(yuid)
                f.write(yuid)
                f.write("\n")
    logging.info("done %s", yt_path)


def main():
    logging.basicConfig(format='[%(levelname)-7s] %(asctime)s - %(message)s', level=logging.INFO)

    if False:
        pool = multiprocessing.Pool(4)
        exp_dates = utime.DateRange(datetime.date(2016, 1, 12), datetime.date(2016, 1, 24))
        history_dates = utime.DateRange(datetime.date(2015, 12, 29), datetime.date(2016, 1, 11))
        testid = "19607"

        logging.info("get from sessions")
        pool.map(functools.partial(mp_save_yuids_from_sessions,
                                   filename="us"),
                 list(exp_dates))

        logging.info("get from sessions history")
        pool.map(functools.partial(mp_save_yuids_from_sessions,
                                   filename="us_history"),
                 list(history_dates))

        logging.info("get from squeezes")
        pool.map(functools.partial(mp_save_yuids_from_squeezes,
                                   filename="sq",
                                   testid=testid),
                 list(exp_dates))

        logging.info("get from history")
        pool.map(functools.partial(mp_save_yuids_from_squeezes,
                                   testid=testid,
                                   filename="sq_history",
                                   history=exp_dates),
                 list(history_dates))

    if True:
        dates = utime.DateRange(datetime.date(2015, 12, 29), datetime.date(2016, 1, 24))
        with open("yuids") as f:
            yuids = {line.strip() for line in f if line.strip()}
        pool = multiprocessing.Pool(16)
        pool.map(functools.partial(mp_find_yuids_in_sessions,
                                   yuids=yuids),
                 list(dates))


if __name__ == "__main__":
    main()
