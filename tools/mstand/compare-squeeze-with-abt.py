#!/usr/bin/env python2.7
# coding=utf-8

"""
Этот скрипт проверяет совпадение yuid'ов в выжимках стенда и расчетах ABT

Не совпавшие пользователи выгружаются из user_sessions и сохраняются в одну из подпапок в директории us/:
- not_in_abt_no_match - пользователи, которые есть в выжимках, нет в ABT, и не попадают в фильтры libra
    - если такие есть, то что-то явно пошло не так
- not_in_abt_match - пользователи, которые есть в выжимках, нет в ABT, но попадают в фильтры libra
    - это может быть багом ABT
- not_in_stand - пользователи, которые есть в ABT, но нет в выжимках стенда
    - лучше посмотреть глазами и понять, почему они выкидываются
        - известные причины:
            - ABT считает TYandexSiteSearchWebRequest, стенд - нет
- non_web - пользователи из ABT, у которых не было действий в сервисе www.yandex, и в выжимки они не попали
    - есть в ABT, потому что они считают по всему

Также генерируются списки пользователей:
- yuids_(тестид)_(день)_stand.txt и yuids_(тестид)_(день)_abt.txt - выгрузки всех уникальных пользователей из стенда
и ABT соответственно
- yuids_(тестид)_(день)_abt_clean.txt - список пользователей ABT, почищенный от пользователей, которых нет в чистых
user_sessions и пользователей, попавших в категорию non_web

В логи с меткой WARNING пишутся все расхождения, кроме non_web пользователей

Проверка на наблюдении в скрипте показала:
- все пользователи в ABT есть в выжимках
- не все пользователи, которые есть в выжимках, есть в ABT
    - но эти пользователи попадают в фильтр по мнению libra
    - таких пользователей меньше 50 на день*тестид
        - это порядка 0.0001 всех пользователей
"""

import logging
import os
from collections import defaultdict

import mstand_utils.mstand_tables as mstand_tables
import pytlib.client_yt as client_yt
import session_squeezer.squeezer
import yaqutils.json_helpers as ujson
import yaqutils.misc_helpers as umisc
import yaqutils.time_helpers as utime
# noinspection PyPackageRequirements
import yt.wrapper as yt

from mstand_enums.mstand_online_enums import ServiceEnum

YT_CLIENT = client_yt.create_client("hahn", filter_so=True)
# noinspection PyUnresolvedReferences
YT_CLIENT.config["read_retries"]["allow_multiple_ranges"] = True


class AbtReducer(object):
    def __init__(self, fhash, testids):
        self.fhash = fhash
        self.testids = testids

    def __call__(self, key, rows):
        found_testids = set()
        for row in rows:
            value = row["value"]
            value = ujson.load_from_str(value)
            boxies = value["boxies"]
            for boxie in boxies:
                flows = boxie["flows"]
                for flow in flows:
                    flow = flow.strip()
                    flow_split = flow.split()
                    if len(flow_split) == 3:
                        fhash, testid, bucket = flow_split
                        if fhash == self.fhash and testid in self.testids:
                            found_testids.add(testid)

        if found_testids:
            yield {"yuid": key["key"], "testids": list(found_testids)}


def get_yuids_abt_one_day(observation, date):
    testid_yuids = defaultdict(set)
    date_str = utime.format_date(date)
    uids_path = "//home/abt/daily/" + date_str + "/main/main/uids"

    with yt.TempTable("//tmp", "mstand_collect_abt_yuids", client=YT_CLIENT) as tmp:
        all_tables = sorted([uids_path + "/" + version for version in yt.list(uids_path)])
        logging.info("collecting yuids for date %s from tables: %s", date, all_tables)
        # noinspection PyProtectedMember
        yt.run_reduce(
            AbtReducer(fhash=observation.filters._filter_hash, testids=observation.all_testids()),
            all_tables,
            tmp,
            spec={
                "data_size_per_job": 200 * 1024 * 1024,
                "reducer": {"enable_input_table_index": True},
            },
            client=YT_CLIENT,
            reduce_by=["key"],
            format=yt.YsonFormat(control_attributes_mode="row_fields"),
        )

        logging.info("reading data from table %s", tmp)
        for idx, line in enumerate(yt.read_table(tmp, client=YT_CLIENT)):
            if idx % 500 == 0:
                logging.info("processed %s rows", idx)
            for testid in line["testids"]:
                testid_yuids[testid].add(line["yuid"])

        return testid_yuids


def get_yuids_stand_testid_day(testid, fhash, date):
    yuids = set()

    squeeze_path = mstand_tables.MstandDailyTable(
        day=date,
        squeeze_path="//home/mstand/squeeze",
        service=ServiceEnum.IMAGES,
        testid=testid,
        filter_hash=fhash,
        dates=utime.DateRange(date, date),
    ).path

    logging.info("getting yuids from mstand squeeze %s", squeeze_path)

    data = yt.read_table(squeeze_path, client=YT_CLIENT)
    for idx, row in enumerate(data):
        if idx % 500 == 0:
            logging.info("loaded %s records", idx)
        yuids.add(row["yuid"])

    return yuids


def get_yuids_stand_one_day(observation, date):
    data = {}
    for experiment in observation.all_experiments(include_control=True):
        data[experiment.testid] = get_yuids_stand_testid_day(experiment.testid, observation.filters.filter_hash, date)
    return data


def make_file_path(testid, date, suffix):
    return "yuids_{}_{}_{}.txt".format(testid, utime.format_date(date), suffix)


def save_one_day(date, testid_yuids, suffix):
    for testid, yuids in testid_yuids.iteritems():
        with open(make_file_path(testid, date, suffix), "w") as fd:
            for yuid in sorted(yuids):
                fd.write(yuid + "\n")


def save_all(date_testid_yuids, suffix):
    for date, testid_yuids in date_testid_yuids.iteritems():
        save_one_day(date, testid_yuids, suffix)


def load_testid_day(testid, date, suffix):
    yuids = set()
    for yuid in open(make_file_path(testid, date, suffix)):
        yuid = yuid.strip()
        if yuid:
            yuids.add(yuid)
    return yuids


def load_observation_day(observation, date, suffix):
    data = {}
    for experiment in observation.all_experiments(include_control=True):
        try:
            data[experiment.testid] = load_testid_day(experiment.testid, date, suffix)
        except IOError:
            pass
    return data


def get_or_load_yuids(observation, suffix, loader):
    date_testid_yuids = {}
    for date in observation.dates:
        cached = load_observation_day(observation, date, suffix)
        if cached:
            date_testid_yuids[date] = cached
        else:
            date_testid_yuids[date] = loader(observation, date)
    return date_testid_yuids


def get_yuids_abt(observation):
    return get_or_load_yuids(observation, "abt", get_yuids_abt_one_day)


def get_yuids_stand(observation):
    return get_or_load_yuids(observation, "stand", get_yuids_stand_one_day)


def split_chunks(yuids, chunk_size=50):
    for idx in range(0, len(yuids), chunk_size):
        start = idx
        end = min(idx + chunk_size, len(yuids))
        chunk = yuids[start:end]
        umisc.log_progress("progress", end, len(yuids))
        yield chunk


def load_user_sessions_slow(date, yuids):
    def get_sessions_by_yuids(rec):
        if rec["key"] in yuids:
            yield rec

    date_str = utime.format_date(date, pretty=True)
    path = "//user_sessions/pub/search/daily/" + date_str + "/clean"
    tmp = "//home/mstand/.test/abt_yuids_not_in_stand"
    # noinspection PyPackageRequirements
    import yt.wrapper.common as yt_common
    yt.run_map(
        binary=get_sessions_by_yuids,
        source_table=path,
        destination_table=tmp,
        client=YT_CLIENT,
        ordered=True,
        spec={
            "max_failed_job_count": 10,
            "data_size_per_job": yt_common.GB,
            "mapper": {"enable_input_table_index": True},
        },
        format=yt.YsonFormat(control_attributes_mode="row_fields"),
    )

    yuid_to_session = defaultdict(list)

    for row in yt.read_table(tmp, client=YT_CLIENT):
        yuid_to_session[row["key"]].append(row)

    return yuid_to_session


def load_user_sessions(date, yuids, force_all=True):
    logging.info("loading user_sessions for %s yuids", len(yuids))
    all_yuids = sorted(yuids)

    if len(all_yuids) > 100:
        if force_all:
            logging.warning("too many sessions requested and force_all=True, using slow way")
            return load_user_sessions_slow(date, yuids)
        else:
            import random
            logging.warning("too many sessions requested, only downloading 100")
            all_yuids = random.sample(all_yuids, 100)

    date_str = utime.format_date(date, pretty=True)
    yuid_to_session = defaultdict(list)

    for chunk in split_chunks(all_yuids):
        path = "//user_sessions/pub/search/daily/" + date_str + "/clean[" + ",".join(chunk) + "]"
        rows = yt.read_table(path, client=YT_CLIENT)
        for row in rows:
            yuid_to_session[row["key"]].append(row)

    return yuid_to_session


def save_user_session(base_dir, date, testid, yuid, session):
    base_dir = "us/" + base_dir
    if not os.path.exists(base_dir):
        os.makedirs(base_dir)
    fname = base_dir + "/{}_{}_{}.sjson".format(utime.format_date(date), testid, yuid)
    with open(fname, "w") as fd:
        for entry in session:
            ujson.dump_to_fd(entry, fd)
            fd.write("\n")

    return fname


def clean_abt_yuids(testid, date, abt, stand):
    diff = abt - stand

    not_y = set()

    for yuid in diff:
        if not yuid.startswith("y"):
            not_y.add(yuid)

    logging.info("found %s not-y yuids, removing", len(not_y))

    diff = diff - not_y

    logging.info("removing fraud/robots/non-images")

    fraud = set()
    non_images = set()

    diff_list = sorted(diff)

    sessions = load_user_sessions(date, diff_list, force_all=True)

    for yuid in diff_list:
        if yuid not in sessions:
            fraud.add(yuid)
        else:
            logging.info("user %s is in clean user_sessions, checking service", yuid)

            if libra_check_service(sessions[yuid]):
                fname = save_user_session("not_in_stand", date, testid, yuid, sessions[yuid])
                logging.info("- has web requests, saved session to %s", fname)
            else:
                fname = save_user_session("non_images", date, testid, yuid, sessions[yuid])
                logging.info("- doesn't have web requests, saved session to %s", fname)
                non_images.add(yuid)

    clean = abt - fraud - non_images - not_y

    logging.warning("total fraud users: %s, nonweb: %s", len(fraud), len(non_images))
    logging.warning("extra users without fraud/robots: %s", len(diff - fraud - non_images))
    logging.warning("extra yuids: %s", sorted(diff - fraud - non_images))

    return clean


class Record(object):
    def __init__(self, row):
        self.key = row["key"]
        self.subkey = row["subkey"]
        self.value = row["value"]


# noinspection PyUnresolvedReferences,PyPackageRequirements
def libra_check_service(session):
    import libra

    recs = [Record(row) for row in session]

    for request in libra.ParseSession(recs, "blockstat.dict"):
        # if request.IsA("TYandexWebRequest"):
        if request.IsA("TImagesRequestProperties"):
            return True

    return False


# noinspection PyUnresolvedReferences,PyPackageRequirements
def libra_check_filters(session, libra_filter):
    if not libra_filter:
        return True

    import libra

    recs = [Record(row) for row in session]

    for request in libra.ParseSession(recs, "blockstat.dict"):
        if libra_filter.Filter(request):
            return True

    return False


def check_stand_only_yuids(abt, stand, observation):
    if observation.filters:
        libra_filter = session_squeezer.squeezer.libra_create_filter(observation.filters)
    else:
        libra_filter = None

    for date in observation.dates:
        for experiment in observation.all_experiments(include_control=True):
            abt_yuids = abt[date][experiment.testid]
            stand_yuids = stand[date][experiment.testid]

            sessions = load_user_sessions(date, stand_yuids - abt_yuids)
            for yuid, session in sessions.iteritems():
                logging.info("yuid %s is not in abt, checking filters", yuid)
                if libra_check_filters(session, libra_filter):
                    fname = save_user_session("not_in_abt_match", date, experiment.testid, yuid, session)
                    logging.info("check passed, saved to %s", fname)
                else:
                    fname = save_user_session("not_in_abt_no_match", date, experiment.testid, yuid, session)
                    logging.info("check not passed, saved to %s", fname)

            write_summary(experiment.testid, date, abt_yuids, stand_yuids)


def format_long(yuids):
    yuids = sorted(yuids)
    if len(yuids) > 10:
        return str(len(yuids)) + "total: " + ", ".join(yuids[:5]) + "..." + ", ".join(yuids[-5:])
    else:
        return ", ".join(yuids)


def write_summary(testid, date, abt, stand):
    logging.warning("summary for testid %s on date %s", testid, date)
    logging.warning("testids not in stand: %s", format_long(abt - stand))
    logging.warning("testids not in abt: %s", format_long(stand - abt))


def clean_all_abt_yuids(observation, abt_date_testid_yuids, stand_date_testid_yuids):
    for date in observation.dates:
        for experiment in observation.all_experiments(include_control=True):
            clean = load_observation_day(observation, date, "abt_clean")
            if clean:
                clean = clean[experiment.testid]
            else:
                clean = clean_abt_yuids(
                    experiment.testid,
                    date,
                    abt_date_testid_yuids[date][experiment.testid],
                    stand_date_testid_yuids[date][experiment.testid]
                )

            abt_date_testid_yuids[date][experiment.testid] = clean


OBS_ID = "33849"


def main():
    umisc.configure_logger()

    # noinspection PyPackageRequirements,PyUnresolvedReferences
    import libra
    libra.InitFilterFactory("geodata4.bin")
    libra.InitUatraits("browser.xml")

    import adminka.ab_cache as adminka_cache
    import adminka.fetch_observations as adminka_fetch_obs

    ab_session = adminka_cache.AdminkaCachedApi()
    observation = list(adminka_fetch_obs.get_observations(ab_session, [OBS_ID]))[0]

    import datetime
    observation.dates.end = observation.dates.start = datetime.date(2016, 9, 24)

    abt_date_testid_yuids = get_yuids_abt(observation)
    save_all(abt_date_testid_yuids, "abt")

    stand_date_testid_yuids = get_yuids_stand(observation)
    save_all(stand_date_testid_yuids, "stand")

    clean_all_abt_yuids(observation, abt_date_testid_yuids, stand_date_testid_yuids)
    save_all(abt_date_testid_yuids, "abt_clean")

    check_stand_only_yuids(
        abt_date_testid_yuids,
        stand_date_testid_yuids,
        observation
    )


if __name__ == "__main__":
    main()
