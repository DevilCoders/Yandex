#!/usr/bin/env python3
from typing import List, Union, Optional, Iterable, NamedTuple
from datetime import datetime
import logging
import logging.handlers
import subprocess
import raven
import argparse
import yaml
import re

log = logging.getLogger(__name__)


class AptlyExecutionFailed(Exception):
    pass


APTLY_PER_COMMAND_TIMEOUT_SECONDS = 60 * 60 * 24


def aptly(command: Union[List[str], str]) -> str:
    """
    Run aptly command and return it's stdout
    """
    if isinstance(command, str):
        command = command.split()

    command = ["aptly"] + command
    fancy_command = "'%s'" % " ".join(command)
    log.debug("executing %s", fancy_command)
    with subprocess.Popen(
        command,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        encoding="utf-8",
        errors="replace",
    ) as proc:
        try:
            stdout, stderr = proc.communicate(timeout=APTLY_PER_COMMAND_TIMEOUT_SECONDS)
        except subprocess.TimeoutExpired as exc:
            log.warning(
                "command %s timed out: %s. Try to kill it before giving up",
                fancy_command,
                exc,
            )
            try:
                proc.kill()
            except BaseException as kill_exc:
                log.warning("got error while try kill timed out %s: %s", fancy_command, kill_exc)
            raise AptlyExecutionFailed(f"{fancy_command} timed out: {exc}") from exc

        if proc.returncode != 0:
            raise AptlyExecutionFailed(f"{fancy_command} exit with code {proc.returncode}: {stderr}")
        return stdout


def create_mirror(mirror_name: str, url: str, filters: List[str]) -> None:
    filter_arg = "-filter=" + "|".join(sorted(filters))
    all_mirrors = aptly("mirror list -raw").split("\n")
    if mirror_name in all_mirrors:
        aptly(["mirror", "edit", filter_arg, mirror_name])
        return
    log.info("mirror %s not exists. Create it", mirror_name)
    aptly(["mirror", "create", filter_arg, "-filter-with-deps", mirror_name, url, "./"])


def update_mirror(mirror_name: str) -> None:
    aptly(f"mirror update {mirror_name}")


def create_snapshot(mirror_name: str, snapshot_name) -> None:
    aptly(f"snapshot create {snapshot_name} from mirror {mirror_name}")


class PublishedSnapshot(NamedTuple):
    snapshot: str
    mirror: str


def parse_publish_list_out(out: str) -> Iterable[PublishedSnapshot]:
    # Published repositories:
    # ... publishes {main: [mirror-name-2021-04-16T04:32:37.2]: Snapshot from mirror [mirror-name]: http: ...
    for line in out.splitlines():
        match = re.search(
            r"publishes .main: \[(?P<snapshot>[\w:.-]+)\]: Snapshot from mirror \[(?P<mirror>[\w-]+)\]",
            line,
        )
        if match is not None:
            yield PublishedSnapshot(
                snapshot=match.group("snapshot"),
                mirror=match.group("mirror"),
            )


def find_published_snapshot_in_mirror(mirror_name: str) -> Optional[str]:
    out = aptly("publish list")
    for ps in parse_publish_list_out(out):
        if ps.mirror == mirror_name:
            return ps.snapshot
    log.debug("%s published snapshot not found in %s", mirror_name, out)
    return None


def snapshots_are_identical(snapshot_a, snapshot_b) -> bool:
    out = aptly(f"snapshot diff {snapshot_a} {snapshot_b}")
    if out.strip() == "Snapshots are identical.":
        return True
    log.info("snapshots %s and %s have differences\n %s", snapshot_a, snapshot_b, out)
    return False


def publish_snapshot(snapshot_name: str, bucket: str, architectures: List[str]) -> None:
    cmd = ["publish", "snapshot"]
    if architectures:
        cmd.append(f"-architectures={','.join(architectures)}")
    cmd += [snapshot_name, f"s3:{bucket}:"]
    aptly(cmd)


def publish_switch(new_snapshot: str, bucket: str) -> None:
    # Use `-force-overwrite`,
    # cause sometimes we re-upload packages with the same version.
    # It's better not fails on them.
    aptly(f"publish switch -force-overwrite .- s3:{bucket}: {new_snapshot}")


def drop_snapshot(snapshot_name: str) -> None:
    aptly(f"snapshot drop {snapshot_name}")


def sync_mirror(
    mirror_name: str,
    url: str,
    filters: List[str],
    bucket: str,
    architectures: List[str],
) -> None:
    create_mirror(mirror_name, url, filters)
    update_mirror(mirror_name)
    new_snapshot = f"{mirror_name}-{datetime.now().isoformat()}"
    create_snapshot(mirror_name, new_snapshot)
    published_snapshot = find_published_snapshot_in_mirror(mirror_name)
    if published_snapshot is None:
        log.warning(
            "No published snapshots for %r mirror found. Probably it's initial sync",
            mirror_name,
        )
        publish_snapshot(new_snapshot, bucket, architectures)
        return
    if snapshots_are_identical(published_snapshot, new_snapshot):
        log.debug("dropping new %s snapshot, cause its identical to published %s", new_snapshot, published_snapshot)
        drop_snapshot(new_snapshot)
        return
    publish_switch(new_snapshot, bucket)
    log.debug("dropping old %s snapshot", published_snapshot)
    drop_snapshot(published_snapshot)


def main():
    def init_logging():
        log.setLevel(logging.DEBUG)
        file_handler = logging.FileHandler("/var/log/mdb-dist-sync/dist-sync.log")
        file_handler.setFormatter(logging.Formatter("%(asctime)s %(funcName)s [%(levelname)s]: %(message)s"))
        log.addHandler(file_handler)
        if not args.quiet:
            log.addHandler(logging.StreamHandler())

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-c",
        "--config",
        help="path to config",
        type=argparse.FileType("r"),
        default="/etc/yandex/mdb-dist-sync/dist-sync.yaml",
    )
    parser.add_argument("-n", "--no-sentry", help="disable sentry reports", action="store_true")
    parser.add_argument("-q", "--quiet", help="don't print debug to stdout", action="store_true")

    args = parser.parse_args()
    init_logging()

    try:
        cfg = yaml.load(args.config)
        sentry_cfg = cfg.get("sentry", {})
        if sentry_cfg.get("dsn") and not args.no_sentry:
            raven.Client(
                dsn=sentry_cfg["dsn"],
                environment=sentry_cfg.get("environment"),
            )
        for mirror in cfg["mirrors"]:
            sync_mirror(
                mirror_name=mirror["bucket"],
                url=mirror["source"],
                filters=cfg["packages"],
                bucket=mirror["bucket"],
                architectures=mirror.get("architectures", []),
            )
    except BaseException:
        log.exception("unhandled exception")
        raise


if __name__ == "__main__":
    main()
