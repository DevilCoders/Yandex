import mimetypes
import os
import tempfile
from typing import Dict
from typing import List

import boto3
from werkzeug.datastructures import FileStorage
from yc_common import config
from yc_common import logging
from yc_common.clients.kikimr.client import _KikimrTxConnection
from yc_common.clients.kikimr.sql import SqlIn
from yc_common.misc import timestamp
from yc_common.validation import ResourceIdType

from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.db.models import avatars_table
from cloud.marketplace.common.yc_marketplace_common.db.models import marketplace_db
from cloud.marketplace.common.yc_marketplace_common.models.avatar import Avatar as AvatarScheme
from cloud.marketplace.common.yc_marketplace_common.models.avatar import AvatarResponse
from cloud.marketplace.common.yc_marketplace_common.models.task import PublishLogoParams
from cloud.marketplace.common.yc_marketplace_common.utils import errors
from cloud.marketplace.common.yc_marketplace_common.utils.transactions import mkt_transaction

log = logging.get_logger(__name__)


def get_avatar_internal_host() -> str:
    return config.get_value("endpoints.s3.url")


VALID_MIME_TYPES = {
    "image/svg+xml",
}


class Avatar:
    @staticmethod
    def rpc_create(input_files: Dict[str, FileStorage]) -> AvatarResponse:
        input_file = None
        if "avatar" in input_files:
            input_file = input_files["avatar"]

        if input_file is None or input_file.filename == "":
            raise errors.ImageFieldRequired()
        filename = input_file.filename
        if "." not in filename:
            raise errors.MalformedFilename()

        new_avatar = Avatar.create(filename, input_file.stream)

        return new_avatar.to_public()

    @staticmethod
    @mkt_transaction()
    def create(filename, input_file, *, tx=None):
        mimetype, _ = mimetypes.guess_type(filename)
        check_picture_mimetype(mimetype)
        aws_access_key_id = os.getenv("PICTURE_S3_ACCESS_KEY")
        aws_secret_access_key = os.getenv("PICTURE_S3_SECRET_KEY")
        if aws_access_key_id is None or aws_secret_access_key is None:
            raise errors.S3UploadCredentialsNotSet()

        ext = filename.split(".")[-1]
        new_avatar = AvatarScheme.new()
        session = boto3.session.Session()
        client = session.client(
            service_name="s3",
            endpoint_url=config.get_value("endpoints.s3.url"),
            aws_access_key_id=aws_access_key_id,
            aws_secret_access_key=aws_secret_access_key,
        )
        filename = "{}.{}".format(new_avatar.id, ext)
        try:
            client.upload_fileobj(input_file, config.get_value("endpoints.s3.default_bucket"), filename,
                                  ExtraArgs={"ContentType": mimetype})
        except Exception as e:
            log.error("failed to upload image = {}".format(e))
            raise errors.ImageUploadError()
        url = "{}/{}/{}".format(get_avatar_internal_host(), config.get_value("endpoints.s3.default_bucket"), filename)
        new_avatar.meta = {
            "url": url,
            "key": new_avatar.id,
            "bucket": config.get_value("endpoints.s3.default_bucket"),
            "ext": ext,
        }
        tx.with_table_scope(avatars_table).insert_object("INSERT INTO $table", new_avatar)
        return new_avatar

    @staticmethod
    def rpc_copy(avatar_id: str, target_key: str, target_bucket: str, rewrite: bool = False) -> str:
        aws_access_key_id = os.getenv("PICTURE_S3_ACCESS_KEY")
        aws_secret_access_key = os.getenv("PICTURE_S3_SECRET_KEY")
        if any(v is None for v in (aws_access_key_id, aws_secret_access_key)):
            raise EnvironmentError("Environment variables are not set")
        pict_dict = Avatar.rpc_get_by_ids([avatar_id])
        meta = pict_dict[avatar_id].meta

        session = boto3.session.Session()
        client = session.client(
            service_name="s3",
            endpoint_url=config.get_value("endpoints.s3.url"),
            aws_access_key_id=aws_access_key_id,
            aws_secret_access_key=aws_secret_access_key,
        )
        target_filename = "{}.{}".format(target_key, meta["ext"])
        source_filename = "{}.{}".format(meta["key"], meta["ext"])

        try:
            with tempfile.TemporaryFile() as data:
                if rewrite:
                    client.delete_object(Bucket=target_bucket, Key=target_filename)
                client.download_fileobj(meta["bucket"], source_filename, data)
                data.seek(0)
                mimetype, _ = mimetypes.guess_type(target_filename)
                check_picture_mimetype(mimetype)
                client.upload_fileobj(data, target_bucket, target_filename, ExtraArgs={"ContentType": mimetype})
        except Exception as e:
            log.error("failed to copy image = {}".format(e))
            raise errors.ImageUploadError()

        url = "{}/{}/{}".format(get_avatar_internal_host(), target_bucket, target_filename)
        return url

    @staticmethod
    def _get_image(tx: _KikimrTxConnection, image_id: ResourceIdType) -> AvatarScheme:
        return tx.select_one("SELECT " + AvatarScheme.db_fields() + " " +
                             "FROM $table "
                             "WHERE id = ?", image_id,
                             model=AvatarScheme)

    @staticmethod
    @mkt_transaction()
    def capture_image(image_id: ResourceIdType, object_id: str, *, tx) -> AvatarScheme:
        tx = tx.with_table_scope(avatars_table)

        avatar = Avatar._get_image(tx, image_id)

        now = timestamp()

        tx.update_object("UPDATE $table $set WHERE id = ?", {
            "status": AvatarScheme.Status.LINKED,
            "updated_at": now,
            "linked_object": object_id,
        }, avatar.id)
        avatar.status = AvatarScheme.Status.LINKED
        avatar.updated_at = now
        avatar.linked_object = object_id

        return avatar

    @staticmethod
    def rpc_get_by_ids(ids: List[str]) -> Dict[str, AvatarScheme]:
        with marketplace_db().with_table_scope(avatars_table).transaction() as tx:
            res = tx.select("SELECT " + AvatarScheme.db_fields() + " " +
                            "FROM $table "
                            "WHERE ?", SqlIn("id", ids),
                            model=AvatarScheme)
            return {image.id: image for image in res}

    @staticmethod
    def get_image(image_id: ResourceIdType) -> AvatarResponse:
        with marketplace_db().with_table_scope(avatars_table).transaction() as tx:
            avatar = Avatar._get_image(tx, image_id)

        if avatar is None:
            raise errors.AvatarIdError()

        return avatar.to_public()

    @staticmethod
    @mkt_transaction()
    def task_publish(avatar_id: str, target_key: str, target_bucket: str, group_id: str,
                     depends: list = None, rewrite=False, *, tx):
        if depends is None:
            depends = []
        return lib.TaskUtils.create(
            "publish_logo",
            group_id=group_id,
            tx=tx,
            params=PublishLogoParams({
                "avatar_id": avatar_id,
                "target_key": target_key,
                "target_bucket": target_bucket,
                "rewrite": rewrite,
            }).to_primitive(),
            depends=depends,
        )


def check_picture_mimetype(mimetype: str):
    if mimetype not in VALID_MIME_TYPES:
        raise errors.ImageInvalidMimeType()
