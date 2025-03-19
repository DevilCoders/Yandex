#! /usr/bin/env python3


import os

from datetime import datetime
from pytz import timezone


from cloud.netinfra.rknfilter.yc_rkn_common.utils import create_logger_object

from cloud.netinfra.rknfilter.yc_rkn_common.local_files import enlist_local_files_and_calculate_hashes
from .s3remote_objects import enlist_existing_s3_objects
from .s3connect import get_s3_bucket_context


def upload_files_to_s3(
    s3_bucket, s3_key_prefix, files_locations=list(), transaction_id=None, logger=None
):
    if logger:
        logger.info("preparing to upload backup and actual configuration files")
    local_files_with_hashes = enlist_local_files_and_calculate_hashes(
        files_locations, logger=logger
    )
    s3_objects = enlist_existing_s3_objects(
        s3_bucket, s3_key_prefix, files_locations, logger=logger
    )
    if logger:
        logger.info("upload preparations are finished")
    if not transaction_id:
        transaction_id = datetime.fromtimestamp(0, tz=timezone("Europe/Moscow"))
    perform_upload_transaction(
        s3_bucket,
        s3_key_prefix,
        local_files_with_hashes["file_names"],
        s3_objects["s3_objects"],
        local_files_with_hashes["existing_files"],
        transaction_id,
        logger=logger,
    )
    if logger and local_files_with_hashes:
        logger.info("uploading is finished")
    elif logger and not local_files_with_hashes:
        logger.info("There was nothing to upload")


def perform_upload_transaction(
    s3_bucket,
    s3_key_prefix,
    file_names,
    s3_objects,
    local_files_with_hashes,
    transaction_id,
    logger=None,
):
    if logger:
        logger.debug("Running upload transaction")
        logger.debug("Transaction id is: {}".format(transaction_id.isoformat()))
    for file_name in file_names:
        upload_file_to_s3(
            s3_bucket,
            s3_key_prefix,
            file_name,
            s3_objects,
            local_files_with_hashes,
            transaction_id,
            logger=logger,
        )
    if logger:
        logger.debug("Upload transaction is finished")


def upload_file_to_s3(
    s3_bucket,
    s3_key_prefix,
    file_name,
    s3_objects,
    existing_files,
    transaction_id,
    logger=None,
):
    s3_object_key = os.path.join(s3_key_prefix, file_name)
    to_upload_or_not_to_upload = make_upload_decision(
        file_name,
        s3_object_key,
        s3_objects,
        existing_files,
        transaction_id,
        logger=logger,
    )
    if to_upload_or_not_to_upload:
        s3_object_upload_extraargs = {
            "Metadata": {
                "Hash": existing_files[file_name].file_hash,
                "Transaction_id": transaction_id.isoformat(),
            }
        }
        s3_bucket.upload_file(
            existing_files[file_name].file_path,
            s3_object_key,
            ExtraArgs=s3_object_upload_extraargs,
        )
        if logger:
            logger.debug("Uploading of file is finished")


def make_upload_decision(
    file_name, s3_object_key, s3_objects, existing_files, transaction_id, logger=None
):
    to_upload_or_not_to_upload = True
    if file_name in s3_objects and file_name in existing_files:
        if s3_objects[file_name].transaction_id == transaction_id:
            if logger:
                logger.debug(
                    "S3 object {} transaction_id matches with transaction_id which is going to be assigned to: {} \n"
                    "S3 object is up to date".format(
                        s3_objects[file_name].s3_object_key,
                        existing_files[file_name].file_path,
                    )
                )
            to_upload_or_not_to_upload = False
        elif s3_objects[file_name].transaction_id < transaction_id:
            if logger:
                logger.debug(
                    "S3 object {} transaction_id older then transaction_id which is going to be assigned to: {} \n"
                    "S3 object seems to outdated".format(
                        s3_objects[file_name].s3_object_key,
                        existing_files[file_name].file_path,
                    )
                )
        elif s3_objects[file_name].transaction_id > transaction_id:
            if logger:
                logger.debug(
                    "S3 object {} transaction_id newer then transaction_id which is going to be assigned to: {} \n"
                    "Local file seems to outdated".format(
                        s3_objects[file_name].s3_object_key,
                        existing_files[file_name].file_path,
                    )
                )
    elif file_name in s3_objects:
        if logger:
            logger.debug(
                "File {} does not present on local filesystem".format(
                    s3_objects[file_name].file_path
                )
            )
            logger.debug(
                "S3 object {} will not be overriden".format(
                    s3_objects[file_name].s3_object_key
                )
            )
        to_upload_or_not_to_upload = False
    elif file_name in existing_files and file_name not in s3_objects:
        if logger:
            logger.debug(
                "Respective S3 object does not exist: {} \n"
                "Uploading ...".format(s3_object_key)
            )
    else:
        if logger:
            logger.debug(
                "Nothing going to be done with S3 object or Local file relevant to: {} ".format(
                    file_name
                )
            )
        to_upload_or_not_to_upload = False
    return to_upload_or_not_to_upload


if __name__ == "__main__":
    ENDPOINT_URL = "http://s3.mdst.yandex.net"
    ACCESS_KEY_ID = "BoHLgghFgKtxrFmmWcds"
    SECRET_ACCESS_KEY = "8U3JNhy2YZ3K3BYj7K8tZnQxUCsf6vuwzXujtbVE"
    S3_BUCKET_NAME = "netinfra-rkn"
    s3_key_prefix = "Current"
    files_locations = [
        "/var/opt/yc/rkn/config_node/react.rules",
        "/var/opt/yc/rkn/config_node/bird.conf.static_routes",
    ]

    LOGGER_NAME = __name__
    LOG_FILE = "/var/log/rkn-S3-operations.log"
    logger = create_logger_object(LOG_FILE, LOGGER_NAME, False, True)
    s3_bucket = get_s3_bucket_context(
        ENDPOINT_URL, ACCESS_KEY_ID, SECRET_ACCESS_KEY, S3_BUCKET_NAME, logger
    )
    upload_files_to_s3(s3_bucket, s3_key_prefix, files_locations, logger=logger)
