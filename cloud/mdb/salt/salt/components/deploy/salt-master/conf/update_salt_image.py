#!/usr/bin/env python3

from argparse import ArgumentParser
import configparser
from datetime import timedelta, datetime
import logging
import logging.handlers
import os
from pathlib import Path
import shutil
import subprocess
from typing import Any, List, NamedTuple
from urllib.parse import urlparse

log = logging.getLogger(__name__)

S3_IMAGES_PREFIX = "image-"


class UpdateImageError(Exception):
    """
    Base update image error
    """


class MountLinkIsBroken(UpdateImageError):
    """
    Mount link /srv points to non existed path
    """


class S3ImagesNotEnabled(UpdateImageError):
    """
    /srv is not symlink
    """


class CommandExecutionError(UpdateImageError):
    """
    Command execution failed
    """


class Config(NamedTuple):
    srv: str
    srv_images: str
    bucket: str
    list_timeout: float


def _get_my_image(config: Config) -> str:
    mount = Path(config.srv)
    if not mount.exists():
        raise MountLinkIsBroken
    if not mount.is_symlink():
        raise S3ImagesNotEnabled
    mount = mount.resolve()
    return mount.name


def _run(cmd_args: List[str], **kwargs) -> str:
    log.debug("executing '%s' with: %r", " ".join(cmd_args), kwargs)
    try:
        cmd = subprocess.run(
            cmd_args,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            encoding="utf-8",
            **kwargs,
        )
        if cmd.returncode:
            raise CommandExecutionError(
                "{cmd} failed code: {code}, stderr: {err}".format(
                    cmd=" ".join(cmd_args), code=cmd.returncode, err=cmd.stderr
                )
            )
        return cmd.stdout
    except subprocess.TimeoutExpired:
        raise CommandExecutionError(
            "{cmd} timed out after {t} seconds".format(cmd=" ".join(cmd_args), t=kwargs.get("timeout"))
        )


Image = NamedTuple("Image", [("s3_path", str), ("date", datetime)])


def _list_images(config: Config) -> List[Image]:
    s3_bucket = config.bucket
    out = _run(
        ["s3cmd", "ls", "s3://" + s3_bucket + "/" + S3_IMAGES_PREFIX],
        timeout=config.list_timeout,
    )
    images = []
    # $ s3cmd ls s3://mdb-salt-images/image-
    # 2020-06-26 11:22   6398956   s3://mdb-salt-images/image-1593170545-r7039480.txz
    # 2020-06-26 11:29   6398436   s3://mdb-salt-images/image-1593170975-r7039480.txz
    for line in out.splitlines():
        parts = line.split()
        date_str = parts[0] + " " + parts[1]
        try:
            date = datetime.strptime(date_str, "%Y-%m-%d %H:%M")
        except ValueError as exc:
            raise CommandExecutionError("failed to parse image date: %s" % exc)
        images.append(Image(parts[-1], date))
    return images


def _download_image_to_images_path(config: Config, s3_image_uri: str) -> None:
    _run(
        ["s3cmd", "get", "--force", s3_image_uri],
        cwd=config.srv_images,
    )


def _extract_image(config: Config, image: str, archive_file_name: str) -> None:
    image_abs_path = Path(config.srv_images) / image
    if image_abs_path.exists():
        log.warning(
            "path '%s' already exists. Looks like retry. Cleanup it first.",
            image_abs_path,
        )
        shutil.rmtree(image_abs_path, ignore_errors=True)
    image_abs_path.mkdir()

    try:
        _run(
            [
                "tar",
                "xJf",
                archive_file_name,
                "--directory=" + image,
                "--strip-components=1",
            ],
            cwd=config.srv_images,
        )
    finally:
        (Path(config.srv_images) / archive_file_name).unlink()


def _update_mount_link(config: Config, image: str) -> None:
    image_abs_path = (Path(config.srv_images) / image).resolve()
    tmp_link = Path(config.srv + ".tmp")
    if tmp_link.exists():
        log.warning("exists temporary link: '%s'. Remove it", tmp_link)
        tmp_link.unlink()

    tmp_link.symlink_to(image_abs_path)
    log.debug("temporary link '%s' created. Rename it to '%s'", tmp_link, config.srv)
    mount = Path(config.srv)
    if mount.exists() and not mount.is_symlink():
        log.warning("'%s' exists and it's not a link. Remove it", config.srv)
        shutil.rmtree(config.srv)
    os.replace(tmp_link, config.srv)


def _remove_image_dir(config: Config, image: str) -> None:
    image_path = Path(config.srv_images) / image
    if image_path.exists() and image_path.is_dir():
        shutil.rmtree(Path(config.srv_images) / image, ignore_errors=True)


def _update_image(config: Config, s3_image_uri: str, old_image: str) -> None:
    srv_images_dir = Path(config.srv_images)
    if not srv_images_dir.exists():
        log.warning("'%s' path not exists. create it", srv_images_dir)
        srv_images_dir.mkdir()

    archive_file_name = urlparse(s3_image_uri).path.lstrip("/")
    image = archive_file_name[: archive_file_name.rfind(".")]
    log.info("updating image to '%s'. (current is %s)", image, old_image)

    _download_image_to_images_path(config, s3_image_uri)
    _extract_image(config, image, archive_file_name)
    _update_mount_link(config, image)
    if old_image and old_image != image:
        _remove_image_dir(config, old_image)

    log.info("salt image updated to '%s'", image)


def _init_logging(args: Any) -> None:
    log.setLevel(logging.DEBUG)
    handler = logging.handlers.SysLogHandler(address="/dev/log")
    handler.setFormatter(logging.Formatter("update_salt_image[%(process)d]: %(message)s"))
    log.addHandler(handler)
    if not args.quiet:
        log.addHandler(logging.StreamHandler())


OK = 0
WARN = 1
CRIT = 2


def _monrun(code: int, message: str) -> None:
    print("%d;%s" % (code, message.replace("\n", " ").replace(";", " ")))


def check_image(args: Any, config: Config) -> None:
    """
    Check image freshness.
    Monrun entry point.
    """
    try:
        my_image = _get_my_image(config)
    except S3ImagesNotEnabled:
        return _monrun(WARN, "salt-master not switched to s3 image (/srv isn't symlink)")
    except MountLinkIsBroken:
        return _monrun(CRIT, "mount link '%s' points to not existed directory" % config.srv)

    try:
        s3_images = _list_images(config)
    except CommandExecutionError as exc:
        return _monrun(WARN, str(exc))

    if not s3_images:
        # it's not a CRIT at that point, cause that case monitored on salt-sync
        return _monrun(WARN, "there are no images in s3")

    if my_image in max(img.s3_path for img in s3_images):
        return _monrun(OK, "'%s' is the latest" % my_image)

    # find minimum img.date that comes 'after' our image
    min_img_date = None
    my_image_found = False
    for img in sorted(s3_images, key=lambda img: img.s3_path):
        if my_image_found:
            if min_img_date is None or min_img_date > img.date:
                min_img_date = img.date
            continue
        if not my_image_found and my_image in img.s3_path:
            my_image_found = True
    if min_img_date is None:
        # there are no our image in s3
        min_img_date = min(img.date for img in s3_images)

    update_delay = datetime.utcnow() - min_img_date
    if update_delay > timedelta(minutes=args.warn):
        code = WARN
        if update_delay > timedelta(minutes=args.crit):
            code = CRIT
        return _monrun(code, "'%s' update delayed by %s" % (my_image, update_delay))

    return _monrun(
        OK,
        "'%s' is not the last one, but it's fresh (%s)" % (my_image, update_delay),
    )


def update_image(args: Any, config: Config) -> None:
    """
    Update /srv from s3 if need it.
    """
    try:
        my_image = _get_my_image(config)
    except S3ImagesNotEnabled:
        if not args.force:
            return
        my_image = ""
    except MountLinkIsBroken:
        my_image = ""
        log.error("%s path not exits. Updating it", config.srv)

    latest_s3_image_path = max(img.s3_path for img in _list_images(config))

    if not args.force:
        if my_image and my_image in latest_s3_image_path:
            log.debug("we use %s latest s3 image %s", my_image, latest_s3_image_path)
            return

    _update_image(config, latest_s3_image_path, my_image)


def _load_config(args: Any) -> Config:
    parser = configparser.ConfigParser()
    parser.read([args.config])
    return Config(
        srv=parser['master']['srv'],
        srv_images=parser['master']['srv_images'],
        bucket=parser['s3']['bucket'],
        list_timeout=parser.getfloat('s3', 'list_timeout', fallback=20.0),
    )


def main():
    parser = ArgumentParser()
    parser.add_argument("-q", "--quiet", action="store_true", help="don't print debug output to stdout")
    parser.add_argument("-c", "--config", default="/etc/yandex/mdb-deploy/update_salt_image.conf")
    subparsers = parser.add_subparsers()

    check_parser = subparsers.add_parser("check", help="check image freshness")
    check_parser.add_argument(
        "-w",
        "--warn",
        type=int,
        help="warn if image update delayed more then given minutes",
        default=5,
    )
    check_parser.add_argument(
        "-c",
        "--crit",
        type=int,
        help="crit if image update delayed more then given minutes",
        default=15,
    )
    check_parser.set_defaults(func=check_image)

    update_parser = subparsers.add_parser("update", help="update image")
    update_parser.add_argument(
        "--force",
        action="store_true",
        help="replace old image anyway",
    )
    update_parser.set_defaults(func=update_image)

    args = parser.parse_args()

    _init_logging(args)
    config = _load_config(args)

    # we have py3.6 only, so can't use required=True in add_subparsers
    if not hasattr(args, "func"):
        parser.print_help()
        parser.error("no action provided")

    try:
        args.func(args, config)
    except:  # noqa
        log.exception("unhandled exception in %s handler", args.func)
        raise


if __name__ == "__main__":
    main()
