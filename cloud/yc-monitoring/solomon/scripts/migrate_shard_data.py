#!/usr/bin/env python3.6
import asyncio
from typing import Any, Dict, List, Optional, Tuple, TextIO, Iterable
from aiohttp import ClientSession, ClientTimeout, ClientOSError
from argparse import ArgumentParser, RawTextHelpFormatter
from datetime import datetime, timedelta
from os import makedirs, rename, environ
from os.path import join as path_join, isfile
from dateutil.parser import parse as parse_date
from dateutil.relativedelta import relativedelta
from json import dump as json_dump, loads as json_loads
from logging import getLogger, basicConfig, INFO as LOGGING_INFO
import time
from threading import Thread
from glob import glob
from copy import deepcopy
from pprint import pformat


basicConfig(format="%(asctime)s %(message)s", datefmt="[%H:%M:%S]")
log = getLogger()
log.setLevel(LOGGING_INFO)

MAX_POINTS_PER_REQUEST = 10000
UPLOAD_MAX_POINTS_PER_REQUEST = 500
MAX_SENSORS_PER_REQUEST = 640
LOG_STATUS_INTERVAL = 3


class Mode:
    UPLOAD = "upload"
    DOWNLOAD = "download"
    ALL = [UPLOAD, DOWNLOAD]


Sensor = Dict[str, str]
SensorList = List[Sensor]


async def http_request(*, method: str, url: str, params: Dict[str, str] = None, data: Any = None,
                       json: Any = None, headers: Dict[str, str] = None, timeout_sec: float = 15 * 60,
                       retries: int = 10, sleep_before_retry_sec: float = 5):
    for _ in range(retries):
        try:
            async with ClientSession() as session:
                async with session.request(method=method, url=url, data=data, params=params, raise_for_status=False,
                                           json=json, headers=headers, timeout=ClientTimeout(total=timeout_sec)) as req:
                    if req.status != 200:
                        log.warning("Non-200 status code: %s", req)
                    return await req.json()
        except ClientOSError as e:
            log.warning("ClientOSError %s", e.strerror)
            await asyncio.sleep(sleep_before_retry_sec)
            sleep_before_retry_sec *= 2
        except TimeoutError:
            log.warning("Timeout on (url: %s, params: %s, json: %s, data %s)", url, params, json, data)
            await asyncio.sleep(sleep_before_retry_sec)
            sleep_before_retry_sec *= 2
    raise TimeoutError()


class UnexpectedAnswer(Exception):
    def __init__(self, **kwargs):
        self.kwargs = kwargs


class SolomonPointsMigrator:
    def __init__(self, *, endpoint: str, project: str, service: str, cluster: str, directory: str,
                 now: datetime, max_connections: int, token: str, mode: Mode):
        self.endpoint = endpoint
        self.project = project
        self.service = service
        self.cluster = cluster
        self.mode = mode
        self.now = now
        self.token = token
        self.directory = directory
        self.max_connections = max_connections

    async def get_sensors(self) -> Optional[Tuple[datetime, SensorList]]:
        headers = {"Authorization": "OAuth {}".format(self.token)}
        params = {
            "pageSize": 999999999,
            "selectors": "cluster={},service={}".format(self.cluster, self.service)
        }
        url = "{}/api/v2/projects/{}/sensors".format(self.endpoint, self.project)
        json = await http_request(method="GET", params=params, headers=headers, url=url)
        if "result" not in json:
            raise UnexpectedAnswer(method="GET", answer=json, url=url, params=params)
        dates = [parse_date(x["createdAt"]) for x in json["result"]]
        if not dates:
            return
        min_date = (min(dates) - relativedelta(days=2)).replace(tzinfo=None)
        return min_date, [x["labels"] for x in json["result"]]

    async def get_points(self, begin: datetime, end: datetime, sensor: Sensor) -> List[Dict]:
        headers = {"Authorization": "OAuth {}".format(self.token)}
        params = {
            "from": "{}Z".format(begin.isoformat()),
            "to": "{}Z".format(end.isoformat()),
            "maxPoints": MAX_POINTS_PER_REQUEST,
            "downsamplingFill": "NONE",
        }
        url = "{}/api/v2/projects/{}/sensors/data".format(self.endpoint, self.project)
        data = ", ".join(["{}='{}'".format(k, v) for k, v in sensor.items()])
        data = "{" + data + "}"
        json = await http_request(method="POST", data=data, params=params, url=url, headers=headers)
        if "vector" not in json:
            raise UnexpectedAnswer(method="POST", answer=json, url=url, params=params, body=data)
        return [x["timeseries"] for x in json["vector"]]

    async def save_points_to_file(self, begin: datetime, end: datetime, sensor: Sensor, file: TextIO):
        points = await self.get_points(begin, end, sensor)
        json_dump(points, file)
        file.write("\n")

    def filename_from_date(self, date: datetime, ext="jsoncsv") -> str:
        return path_join(self.directory, date.strftime("%Y-%m-%d.")) + ext

    async def save_all_points_to_directory(self):
        async def work():
            nonlocal start_date, current_sensor, file
            while current_sensor < len(sensors):
                sensor = sensors[current_sensor]
                current_sensor += 1
                await self.save_points_to_file(start_date, start_date + delta, sensor, file)

        def log_progress():
            started = time.monotonic()
            while 1:
                log.info("Downloading data to %s (request %d of %d, day %d of %d). Elapsed: %s. Left: %s",
                         tmp_filename, current_sensor, len(sensors), current_day, total_days,
                         timedelta(seconds=int(time.monotonic() - started)),
                         timedelta(seconds=int((total_days - current_day) * seconds_per_request)))
                time.sleep(LOG_STATUS_INTERVAL)
        seconds_per_request = 10
        log.info("Downloading points for [project=%s, cluster=%s, service=%s, directory=%s]",
                 self.project, self.cluster, self.service, self.directory)
        makedirs(self.directory, exist_ok=True)
        log.info("Getting sensor list...")
        sensors = await self.get_sensors()
        if not sensors:
            log.info("No points found, exiting...")
            return
        start_date, sensors = sensors
        log.info("Starting date is %s, sensors found: %d", start_date, len(sensors))
        current_sensor = 0
        tmp_filename = self.filename_from_date(start_date, "tmp")
        total_days = (self.now - start_date + relativedelta()).days
        current_day = 0
        delta = relativedelta(days=1)
        log_thread = Thread(target=log_progress, daemon=True)
        log_thread.start()
        while start_date < self.now:
            started = time.monotonic()
            filename = self.filename_from_date(start_date)
            if isfile(filename):
                log.info("File '%s' already exists, skipping download", tmp_filename)
            else:
                with open(tmp_filename, "w") as file:
                    await asyncio.gather(*[work() for _ in range(self.max_connections)])
                rename(tmp_filename, filename)
                seconds_per_request = (seconds_per_request + time.monotonic() - started) / 2
            current_day += 1
            start_date += delta
            tmp_filename = self.filename_from_date(start_date, "tmp")
            current_sensor = 0
        log.info("Done")

    def transform_sensors(self, json: List[Any]) -> List[Any]:
        for sensor in json:
            labels = deepcopy(sensor["labels"])
            labels.pop("project", None)
            labels.pop("cluster", None)
            labels.pop("service", None)
            point = {
                "kind": sensor["kind"],
                "labels": labels,
            }
            timeseries = []
            if "timestamps" in sensor:
                for ts, value in zip(sensor["timestamps"], sensor["values"]):
                    timeseries.append({"ts": ts // 1000, "value": value})
                point["timeseries"] = timeseries
            if "hist" in sensor:
                point["hist"] = deepcopy(sensor["hist"])
            yield point

    async def upload_chunk(self, points: List[Any]):
        headers = {"Authorization": "OAuth {}".format(self.token)}
        params = {"project": self.project, "cluster": self.cluster, "service": self.service}
        url = self.endpoint + "/api/v2/push"
        json = {"sensors": points}

        answer = await http_request(method="POST", json=json, headers=headers, url=url, params=params)

        if answer["status"] != "OK" or answer["sensorsProcessed"] != len(points):
            raise UnexpectedAnswer(method="POST", body=json, url=url, params=params, answer=answer)

    def chunks_from_file(self, filename: str) -> Iterable[List[Any]]:
        with open(filename) as f:
            chunk = []
            for json in map(json_loads, f):
                for point in self.transform_sensors(json):
                    chunk.append(point)
                    if len(chunk) >= UPLOAD_MAX_POINTS_PER_REQUEST:
                        yield chunk
                        chunk = []
            if chunk:
                yield chunk

    async def upload_points(self):
        log.info("Uploading points for [project=%s, cluster=%s, service=%s, directory=%s]",
                 self.project, self.cluster, self.service, self.directory)

        files = sorted(glob(path_join(self.directory, "*.jsoncsv")))
        log.info("Found %d files", len(files))

        files_count = len(files)
        start_time = time.monotonic()

        for file_idx, file in enumerate(files, 1):
            elapsed_sec = int(time.monotonic() - start_time)
            left_sec = int((files_count - file_idx + 1) * (elapsed_sec / (file_idx - 1))) if file_idx > 1 else None

            log.info("Uploading file %r [%d of %d, elapsed %d sec, left %s sec] ...",
                     file, file_idx, files_count, elapsed_sec, left_sec or "?")

            jobs = []
            for idx, chunk in enumerate(self.chunks_from_file(file), 1):
                log.info("  uploading chunk %d (%d points) ...", idx, len(chunk))
                jobs.append(self.upload_chunk(chunk))
            await asyncio.gather(*jobs)

            new_name = file + ".sent"
            log.info("Renaming %r -> %r.", file, new_name)
            rename(file, new_name)


def parse_args():
    parser = ArgumentParser(description="This script downloads/uploads shard points from solomon",
                            formatter_class=RawTextHelpFormatter,
                            epilog="""
             SOLOMON_OAUTH_TOKEN env variable is also required:
             https://solomon.yandex-team.ru/api/internal/auth
        """)
    parser.add_argument("--mode", type=str, required=True, choices=Mode.ALL)
    parser.add_argument("--project", type=str, default="yandexcloud", help="solomon project name")
    parser.add_argument("--service", type=str, required=True, help="solomon service name, e.g 'sys'")
    parser.add_argument("--cluster", type=str, required=True, help="solomon cluster name, e.g 'cloud_prod_oct'")
    parser.add_argument("--directory", metavar="DIR", type=str, required=True, help="place to write to/read from")
    parser.add_argument("--endpoint", metavar="URL", type=str, default="http://solomon.yandex.net", help="solomon")
    parser.add_argument("--max-connections", metavar="INT", type=int, default=25)

    parsed_args = parser.parse_args()
    token = environ.get("SOLOMON_OAUTH_TOKEN")
    if not token:
        print("SOLOMON_OAUTH_TOKEN env variable is required! Go to https://solomon.yandex-team.ru/api/internal/auth")
        exit(1)
    return parsed_args, token


async def main():
    args, token = parse_args()

    migrator = SolomonPointsMigrator(
        endpoint=args.endpoint,
        now=datetime.utcnow(),
        project=args.project,
        service=args.service,
        cluster=args.cluster,
        directory=args.directory,
        max_connections=args.max_connections,
        mode=args.mode,
        token=token
    )
    try:
        if migrator.mode == Mode.UPLOAD:
            await migrator.upload_points()
        else:
            await migrator.save_all_points_to_directory()
    except UnexpectedAnswer as e:
        log.error("Unexpected answer: \n%s", pformat(e.kwargs, width=160, indent=2))


if __name__ == "__main__":
    asyncio.get_event_loop().run_until_complete(main())
