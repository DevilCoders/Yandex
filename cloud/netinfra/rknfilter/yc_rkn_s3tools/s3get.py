#! /usr/bin/env python3

from datetime import datetime
from pytz import timezone

# from .s3utils import create_logger_object
from cloud.netinfra.rknfilter.yc_rkn_common.local_files import enlist_local_files_and_calculate_hashes
from .s3remote_objects import enlist_existing_s3_objects


def download_objects_from_s3(
    s3_bucket, s3_key_prefix, files_locations=dict(), logger=None
):
    if logger:
        logger.info("Preparing to download files")
    local_files_with_hashes = enlist_local_files_and_calculate_hashes(
        files_locations, logger=logger
    )
    s3_objects = enlist_existing_s3_objects(
        s3_bucket, s3_key_prefix, files_locations, logger=logger
    )
    if logger:
        logger.info("Download preparations are finished")
    if logger:
        logger.debug("Running download transaction")
    updated_files, transaction_id = perform_download_transaction(
        s3_bucket,
        s3_key_prefix,
        s3_objects["file_names"],
        s3_objects["s3_objects"],
        local_files_with_hashes["existing_files"],
        logger=logger,
    )
    if logger:
        logger.debug("Download transaction for configuration objects is finished")
    if logger:
        logger.info("S3 object fetching finished")
    return updated_files, transaction_id


def perform_download_transaction(
    s3_bucket,
    s3_key_prefix,
    file_names,
    s3_objects,
    local_files_with_hashes,
    logger=None,
):
    updated_files = dict()
    transaction_elements = dict()
    for file_name in file_names:
        download_file_from_s3(
            s3_bucket,
            s3_key_prefix,
            file_name,
            s3_objects,
            local_files_with_hashes,
            updated_files,
            transaction_elements,
            logger=logger,
        )
    if transaction_elements:
        transaction_id = min(transaction_elements.values())
    else:
        transaction_id = datetime.fromtimestamp(0, tz=timezone("Europe/Moscow"))
    return updated_files, transaction_id


def download_file_from_s3(
    s3_bucket,
    s3_key_prefix,
    file_name,
    s3_objects,
    existing_files,
    updated_files,
    transaction_elements,
    logger=None,
):
    to_download_or_not_to_download = make_download_decision(
        file_name, s3_key_prefix, s3_objects, existing_files, logger=logger
    )
    if to_download_or_not_to_download:
        s3_bucket.download_file(
            s3_objects[file_name].s3_object_key, s3_objects[file_name].file_path
        )
        updated_files.update({file_name: s3_objects[file_name].transaction_id})
        if logger:
            logger.debug("File downloading finished")
    if file_name in s3_objects:
        transaction_elements.update({file_name: s3_objects[file_name].transaction_id})


def make_download_decision(
    file_name, s3_object_key, s3_objects, existing_files, logger=None
):
    to_download_or_not_to_download = True
    if file_name in s3_objects and file_name in existing_files:
        if (
            s3_objects[file_name].modification_date
            <= existing_files[file_name].modification_date
        ):
            if logger:
                logger.debug(
                    "S3 object {} modification date ({}) less or equal than local file {} modification date ({})\n"
                    "local file  is up to date".format(
                        s3_objects[file_name].s3_object_key,
                        s3_objects[file_name].modification_date,
                        existing_files[file_name].file_path,
                        existing_files[file_name].modification_date,
                    )
                )
            to_download_or_not_to_download = False
        else:
            if logger:
                logger.debug(
                    "S3 object ({}) modification date ({}) more than local file ({})modification date ({}) \n"
                    "Local file seems to be outdated. Replacing local file with S3 object".format(
                        s3_objects[file_name].s3_object_key,
                        s3_objects[file_name].modification_date,
                        existing_files[file_name].file_path,
                        existing_files[file_name].modification_date,
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
                "Downloading  S3 object ... : {}".format(
                    s3_objects[file_name].s3_object_key
                )
            )
    elif file_name in existing_files and file_name not in s3_objects:
        if logger:
            logger.debug(
                "Respective S3 object does not exist. Local file {} will not be overriden".format(
                    existing_files[file_name].file_path
                )
            )
        to_download_or_not_to_download = False
    else:
        if logger:
            logger.debug(
                "S3 object or Local file relevant to {} does not exist".format(
                    file_name
                )
            )
        to_download_or_not_to_download = False
    return to_download_or_not_to_download
