#!/usr/bin/env python3


import os
import pandas

from datetime import datetime
from pytz import timezone
from botocore.exceptions import ClientError as s3_client_error

from .s3base import s3_object_resource


def enlist_existing_s3_objects(s3_bucket, s3_key_prefix, files_locations, logger=None):
    if logger:
        logger.info("gathering info about existing s3 objects ...")
    s3_objects, file_names = get_s3_objects_list(
        s3_bucket, s3_key_prefix, files_locations, logger=logger
    )
    s3_objects_list = {"s3_objects": s3_objects, "file_names": file_names}
    return s3_objects_list


def get_s3_objects_list(s3_bucket, prefix, files_locations, logger=None):
    s3_objects_list = dict()
    s3_object_names = set()
    for file_path in files_locations:
        file_name = os.path.basename(file_path)
        s3_object_key = os.path.join(prefix, file_name)
        try:
            if logger:
                logger.info(
                    "trying to fetch hash value and transaction_id value from S3 object metadata: {}".format(
                        s3_object_key
                    )
                )
            s3_object = s3_bucket.Object(s3_object_key)
            s3_object_metadata = s3_object.metadata
            s3_object_modification_date = s3_object.last_modified
            s3_object_modification_date = pandas.to_datetime(
                s3_object_modification_date, utc=True
            )
            s3_object_modification_date = datetime.fromtimestamp(
                s3_object_modification_date.timestamp(), tz=timezone("Europe/Moscow")
            )
            if logger:
                logger.info("S3 object metadata is: {}".format(s3_object_metadata))
            s3_object_hash = s3_object_metadata["Hash"]
            s3_object_transaction_id = s3_object_metadata["Transaction_id"]
            s3_object_transaction_id = pandas.to_datetime(
                s3_object_transaction_id, utc=True
            )
            s3_object_transaction_id = datetime.fromtimestamp(
                s3_object_transaction_id.timestamp(), tz=timezone("Europe/Moscow")
            )
            if logger:
                logger.debug(
                    "creating mapping of file: {} with S3 object : {}".format(
                        file_path, s3_object_key
                    )
                )
            s3_objects_list[file_name] = s3_object_resource(
                file_name,
                file_path,
                s3_object_key,
                s3_object_hash,
                s3_object_transaction_id,
                s3_object_modification_date,
            )
        except s3_client_error:
            if logger:
                logger.debug(
                    "Could not fetch S3 object metadata: {}".format(s3_object_key)
                )
        except KeyError as error_message:
            if logger:
                logger.debug(
                    "Could not find mandatory info S3 object metadata: {}".format(
                        error_message
                    )
                )
        s3_object_names.add(file_name)
    return s3_objects_list, s3_object_names
