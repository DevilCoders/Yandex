#!/usr/bin/env python3

import argparse
import configparser
import logging
import logging.handlers
import time
from typing import List, Optional, TypeVar, Iterable, NamedTuple
import json
import psycopg2
import raven
import requests

log = logging.getLogger(__name__)
T = TypeVar("T")


class ConductorAPI:
    def __init__(self, uri: str):
        self._uri = uri

    def group_children(self, name: str) -> List[str]:
        resp = requests.get(f"{self._uri}/api/groups/{name}?format=json", verify=True)
        resp.raise_for_status()
        # resp[0] - we request one group
        return resp.json()[0]["children"]


class Downtime(NamedTuple):
    downtime_id: str
    end_time: int


def get_downtime_from_response(resp: dict, host: str, description: str) -> Optional[Downtime]:
    """
    Extract downtime from downtimes response.
    Choose downtime with greatest end_time if the response contains more than one downtime.
    """
    ret: Optional[Downtime] = None
    for dt_dict in resp.get("items", []):
        if (
            any(flt.get("host") == host for flt in dt_dict.get("filters"))
            and dt_dict.get("description", "") == description
        ):
            dt = Downtime(dt_dict["downtime_id"], dt_dict["end_time"])
            if ret is None:
                ret = dt
            elif ret.end_time < dt.end_time:
                ret = dt
    return ret


class JugglerAPI:
    _my_downtimes_description = "empty group downtime [MDB-10563]"
    _duration = 3600

    def __init__(self, uri: str, token: str) -> None:
        self._uri = uri
        self._headers = {
            "Authorization": f"OAuth {token}",
            "Accept": "application/json",
            "Content-Type": "application/json",
        }

    def _post(self, method: str, data: dict) -> dict:
        resp = requests.post(
            f"{self._uri}/{method}",
            data=json.dumps(data),
            verify=True,
            headers=self._headers,
        )
        resp.raise_for_status()
        return resp.json()

    def _get_downtime(self, host) -> Optional[Downtime]:
        resp = self._post(
            "v2/downtimes/get_downtimes",
            {
                "filters": [
                    {
                        "host": host,
                        "instance": "",
                        "namespace": "",
                        "service": "",
                        "tags": [],
                    }
                ]
            },
        )
        return get_downtime_from_response(resp, host, self._my_downtimes_description)

    def downtime_exists(self, host: str) -> None:
        """
        Ensure that downtime exists
        """
        now = int(time.time())
        existing = self._get_downtime(host)
        if existing is not None:
            if (existing.end_time - now) > self._duration / 2:
                log.info("shouldn't prolong %r downtime", existing)
                return
            log.info(
                "%s downtime %r already exists, but we should prolong it",
                host,
                existing,
            )
        data = {
            "filters": [{"host": host}],
            "description": self._my_downtimes_description,
            "end_time": now + self._duration,
        }

        if existing:
            data["downtime_id"] = existing.downtime_id
        else:
            data["start_time"] = now

        log.info("setting downtime %r", data)
        self._post("v2/downtimes/set_downtimes", data=data)

    def downtime_absent(self, host: str) -> None:
        """
        Ensure that downtime is absent
        """
        existing = self._get_downtime(host)
        if existing is None:
            log.info("there are no our downtimes on host. Nothing to do.")
            return

        log.info("removing downtime %r", existing)
        self._post("v2/downtimes/remove_downtimes", {"downtime_ids": [existing.downtime_id]})


def vhost_from_conductor_group(group: str) -> str:
    """
    convert conductor group to juggler vhost
    mdb_postgresql_compute_prod -> mdb-postgresql-compute-prod
    """
    return group.replace("_", "-")


def extract_ids_from_group(group: str) -> Optional[str]:
    """
    extract id from it's group.

    For Postgresql, that ids are cids
    For ClickHouse, that ids are subcids.
    """
    prefix = "db_"
    if group.startswith(prefix):
        return group[len(prefix) :]
    return None


def chunks(lst: List[T], chunk_size=100) -> Iterable[List[T]]:
    """
    Split list onto chunks
    """
    for i in range(0, len(lst), chunk_size):
        yield lst[i : i + chunk_size]


CHECK_QUERIES = [
    """
    SELECT cid, type, status
      FROM dbaas.clusters
      JOIN dbaas.folders
     USING (folder_id)
     WHERE cid = ANY(%(ids)s)
       AND folder_ext_id != %(ignore_folder)s
       AND dbaas.visible_cluster_status(clusters.status)
       AND status NOT IN ('STOPPED', 'STOPPING', 'STARTING', 'START-ERROR')
     FETCH FIRST ROW ONLY
    """,
    """
    SELECT cid, type, status
      FROM dbaas.subclusters
      JOIN dbaas.clusters
     USING (cid)
      JOIN dbaas.folders
     USING (folder_id)
     WHERE subcid = ANY(%(ids)s)
       AND folder_ext_id != %(ignore_folder)s
       AND dbaas.visible_cluster_status(clusters.status)
       AND status NOT IN ('STOPPED', 'STOPPING', 'STARTING', 'START-ERROR')
     FETCH FIRST ROW ONLY
    """,
]


def has_working_clusters_not_in_folder(dsn: str, ids: List[str], ignore_folder: str) -> bool:
    with psycopg2.connect(dsn) as conn:
        cur = conn.cursor()
        for ids_part in chunks(ids):
            for query in CHECK_QUERIES:
                cur.execute(
                    query,
                    dict(ignore_folder=ignore_folder, ids=ids_part),
                )
                row = cur.fetchone()
                if row:
                    log.info('There is %r working cluster', row)
                    return True
    return False


def downtime_empty_groups(
    root_group: str,
    e2e_folder: str,
    metadb_dsn: str,
    juggler: JugglerAPI,
    conductor: ConductorAPI,
) -> None:
    for group in conductor.group_children(root_group):
        clusters_groups = conductor.group_children(group)
        ids = [c for c in map(extract_ids_from_group, clusters_groups) if c]
        vhost = vhost_from_conductor_group(group)

        log.debug(
            "examining %r group for %r host. It has %d subgroups",
            group,
            vhost,
            len(ids),
        )
        # check that group have not e2e clusters
        if has_working_clusters_not_in_folder(metadb_dsn, ids, e2e_folder):
            juggler.downtime_absent(vhost)
        else:
            juggler.downtime_exists(vhost)


def main():
    def init_logging():
        log.setLevel(logging.DEBUG)
        file_handler = logging.FileHandler("/var/log/mdb-downtimer/downtimer.log")
        file_handler.setFormatter(logging.Formatter("%(asctime)s %(funcName)s [%(levelname)s]: %(message)s"))
        log.addHandler(file_handler)
        if not args.quiet:
            log.addHandler(logging.StreamHandler())

    parser = argparse.ArgumentParser(
        """Get children groups from conductor and set downtime on empty groups.

Motivation:
    Juggler treat checks on empty groups as invalid (JUGGLERSUPPORT-1754).
    We use ansible-juggler for checks creation, and can't simple recreate checks when cluster created or deleted
""",
    )
    parser.add_argument(
        "-c",
        "--config",
        help="path to config",
        default="/etc/yandex/mdb-downtimer/downtimer.cfg",
    )
    parser.add_argument("-n", "--no-sentry", help="disable sentry reports", action="store_true")
    parser.add_argument("-q", "--quiet", help="don't print debug to stdout", action="store_true")

    args = parser.parse_args()
    init_logging()

    try:
        cfg = configparser.ConfigParser()
        cfg.read([args.config])
        if cfg.get("sentry", "dsn") and not args.no_sentry:
            # Don't need to store sentry client,
            # cause Raven install uncaught exception handler (sys.excepthook).
            # And here we want to send all uncaught exception to Sentry.
            raven.Client(
                dsn=cfg.get("sentry", "dsn"),
                environment=cfg.get("sentry", "environment"),
            )
        juggler = JugglerAPI(
            uri=cfg.get("juggler", "uri"),
            token=cfg.get("juggler", "token"),
        )
        conductor = ConductorAPI(uri=cfg.get("conductor", "uri"))
        downtime_empty_groups(
            root_group=cfg.get("main", "root_group"),
            e2e_folder=cfg.get("main", "e2e_folder"),
            metadb_dsn=cfg.get("metadb", "dsn"),
            juggler=juggler,
            conductor=conductor,
        )
    except BaseException:
        log.exception("unhandled exception")
        raise


if __name__ == "__main__":
    main()
