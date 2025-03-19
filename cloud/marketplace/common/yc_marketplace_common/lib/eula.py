import mimetypes
import os
from typing import Dict
from typing import List

import boto3
from werkzeug.datastructures import FileStorage

from cloud.marketplace.common.yc_marketplace_common import lib
from cloud.marketplace.common.yc_marketplace_common.db.models import eulas_table
from cloud.marketplace.common.yc_marketplace_common.db.models import marketplace_db
from cloud.marketplace.common.yc_marketplace_common.models.eula import Eula as EulaScheme
from cloud.marketplace.common.yc_marketplace_common.models.eula import EulaResponse
from cloud.marketplace.common.yc_marketplace_common.models.task import PublishEulaParams
from cloud.marketplace.common.yc_marketplace_common.utils import errors
from cloud.marketplace.common.yc_marketplace_common.utils.transactions import mkt_transaction
from yc_common import config
from yc_common import logging
from yc_common.clients.kikimr.client import _KikimrTxConnection
from yc_common.clients.kikimr.sql import SqlIn
from yc_common.misc import timestamp
from yc_common.validation import ResourceIdType

log = logging.get_logger('yc')


def get_eula_internal_host() -> str:
    return config.get_value("endpoints.s3.url")


VALID_MIME_TYPES = {
    "application/pdf",
}


class Eula:
    @staticmethod
    def rpc_create(input_files: Dict[str, FileStorage]) -> EulaResponse:
        input_file = None
        if "eula" in input_files:
            input_file = input_files["eula"]

        if input_file is None or input_file.filename == "":
            raise errors.FileFieldRequired()
        filename = input_file.filename
        if "." not in filename:
            raise errors.MalformedFilename()

        new_eula = Eula.create(filename, input_file.stream)

        return new_eula.to_public()

    @staticmethod
    @mkt_transaction()
    def create(filename, input_file, *, tx=None):
        mimetype, _ = mimetypes.guess_type(filename)
        check_file_mimetype(mimetype)
        aws_access_key_id = os.getenv("PICTURE_S3_ACCESS_KEY")
        aws_secret_access_key = os.getenv("PICTURE_S3_SECRET_KEY")
        if aws_access_key_id is None or aws_secret_access_key is None:
            raise errors.S3UploadCredentialsNotSet()

        ext = filename.split(".")[-1]
        new_eula = EulaScheme.new()
        session = boto3.session.Session()
        client = session.client(
            service_name="s3",
            endpoint_url=config.get_value("endpoints.s3.url"),
            aws_access_key_id=aws_access_key_id,
            aws_secret_access_key=aws_secret_access_key,
        )
        filename = "{}.{}".format(new_eula.id, ext)
        try:
            client.upload_fileobj(input_file,
                                  config.get_value("endpoints.s3.default_bucket"),
                                  filename,
                                  ExtraArgs={"ContentType": mimetype})
        except Exception as e:
            log.error("failed to upload image = {}".format(e))
            raise errors.ImageUploadError()
        url = "{}/{}/{}".format(get_eula_internal_host(), config.get_value("endpoints.s3.default_bucket"), filename)
        new_eula.meta = {
            "url": url,
            "key": new_eula.id,
            "bucket": config.get_value("endpoints.s3.default_bucket"),
            "ext": ext,
        }
        tx.with_table_scope(eulas_table).insert_object("INSERT INTO $table", new_eula)
        return new_eula

    @staticmethod
    def rpc_copy(eula_id: str, target_key: str, target_bucket: str) -> str:
        aws_access_key_id = os.getenv("PICTURE_S3_ACCESS_KEY")
        aws_secret_access_key = os.getenv("PICTURE_S3_SECRET_KEY")
        if any(v is None for v in (aws_access_key_id, aws_secret_access_key)):
            raise EnvironmentError("Environment variables are not set")
        pict_dict = Eula.rpc_get_by_ids([eula_id])
        meta = pict_dict[eula_id].meta

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
            client.copy_object(Bucket=target_bucket, Key=target_filename, CopySource={
                "Bucket": meta["bucket"],
                "Key": source_filename,
            })
        except Exception as e:
            log.error("failed to copy image = {}".format(e))
            raise errors.ImageUploadError()

        url = "{}/{}/{}".format(get_eula_internal_host(), target_bucket, target_filename)
        return url

    @staticmethod
    def _get_eula(tx: _KikimrTxConnection, image_id: ResourceIdType) -> EulaScheme:
        return tx.select_one("SELECT " + EulaScheme.db_fields() + " " +
                             "FROM $table "
                             "WHERE id = ?", image_id,
                             model=EulaScheme)

    @staticmethod
    @mkt_transaction()
    def link_agreement(image_id: ResourceIdType, object_id: str, *, tx) -> EulaScheme:
        tx = tx.with_table_scope(eulas_table)

        eula = Eula._get_eula(tx, image_id)

        now = timestamp()

        tx.update_object("UPDATE $table $set WHERE id = ?", {
            "status": EulaScheme.Status.LINKED,
            "updated_at": now,
            "linked_object": object_id,
        }, eula.id)
        eula.status = EulaScheme.Status.LINKED
        eula.updated_at = now
        eula.linked_object = object_id

        return eula

    @staticmethod
    def rpc_get_by_ids(ids: List[str]) -> Dict[str, EulaScheme]:
        with marketplace_db().with_table_scope(eulas_table).transaction() as tx:
            res = tx.select("SELECT " + EulaScheme.db_fields() + " " +
                            "FROM $table "
                            "WHERE ?", SqlIn("id", ids),
                            model=EulaScheme)
            return {file.id: file for file in res}

    @staticmethod
    def get_eula(image_id: ResourceIdType) -> EulaResponse:
        with marketplace_db().with_table_scope(eulas_table).transaction() as tx:
            eula = Eula._get_eula(tx, image_id)

        if eula is None:
            raise errors.EulaIdError()

        return eula.to_public()

    @staticmethod
    @mkt_transaction()
    def task_publish(eula_id: str, target_key: str, target_bucket: str, group_id: str,
                     depends: list = None, *, tx):
        if depends is None:
            depends = []
        return lib.TaskUtils.create(
            "publish_eula",
            group_id=group_id,
            tx=tx,
            params=PublishEulaParams({
                "eula_id": eula_id,
                "target_key": target_key,
                "target_bucket": target_bucket,
            }).to_primitive(),
            depends=depends,
        )


def check_file_mimetype(mimetype: str):
    if mimetype not in VALID_MIME_TYPES:
        raise errors.InvalidFileMimeType()
