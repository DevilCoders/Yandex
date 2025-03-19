#! /usr/bin/env python3


import re

from datetime import datetime, timedelta
from boto3.exceptions import S3UploadFailedError as s3_upload_failed_error


def delete_objects_from_s3(
    s3_bucket,
    only_expired=True,
    s3_object_expiration_days=14,
    s3_object_expiration_hours=0,
    s3_object_expiration_minutes=0,
    logger=None,
):
    try:
        if logger:
            logger.info("Building list of objects to be deleted")
        s3_object_list_to_be_deleted = get_list_of_s3_objects_to_be_deleted(
            s3_bucket,
            only_expired,
            s3_object_expiration_days,
            s3_object_expiration_hours,
            s3_object_expiration_minutes,
            logger=logger,
        )
        if logger:
            logger.info("Building of list of objects to be deleted is finished")
            logger.info(
                "Amount of objects will be deleted: {}".format(
                    len(s3_object_list_to_be_deleted)
                )
            )
            logger.info("Sending deletion request")
        send_deletion_request(s3_bucket, s3_object_list_to_be_deleted, logger=logger)
        if logger:
            logger.info("Deletion of objects is finished")
    except s3_upload_failed_error:
        if logger:
            logger.critical("S3 API connection problems")


def get_list_of_s3_objects_to_be_deleted(
    s3_bucket,
    only_expired,
    s3_object_expiration_days,
    s3_object_expiration_hours,
    s3_object_expiration_minutes,
    logger=None,
):
    s3_object_list_to_be_deleted = list()
    if only_expired:
        if logger:
            logger.debug(
                "Objects older that {} days, {} hours, {} minutes"
                " will be considered as candidates for deletion".format(
                    s3_object_expiration_days,
                    s3_object_expiration_hours,
                    s3_object_expiration_minutes,
                )
            )
        s3_object_expiration_period_as_object = timedelta(
            days=s3_object_expiration_days,
            hours=s3_object_expiration_hours,
            minutes=s3_object_expiration_minutes,
        )
        current_daytime = datetime.now()
    else:
        if logger:
            logger.debug(
                "All S3 objects are going to be considered as candidates for deletion"
            )
    for s3_object in s3_bucket.objects.all():
        s3_object_prefix = get_s3_object_prefix(s3_object.key)
        if not only_expired or (
            re.match("\d{4}-\d{2}-\d{2}-\d{2}-\d{2}-\d{2}", s3_object_prefix)
            and get_s3_object_age(s3_object_prefix, current_daytime)
            >= s3_object_expiration_period_as_object
        ):
            s3_object_list_to_be_deleted.append({"Key": s3_object.key})
            if logger:
                logger.debug(
                    "This S3 object IS considered as candidate for deletion: {}".format(
                        s3_object.key
                    )
                )
        else:
            if logger:
                logger.debug(
                    "This S3 object IS NOT considered as candidate for deletion: {}".format(
                        s3_object.key
                    )
                )
    return s3_object_list_to_be_deleted


def get_s3_object_prefix(s3_object):
    s3_object_prefix, *unused = s3_object.split("/")
    return s3_object_prefix


def get_s3_object_age(s3_object_prefix, current_daytime):
    datetime_args = [int(arg) for arg in s3_object_prefix.split("-")]
    object_creation_datetime_as_object = datetime(*datetime_args)
    return current_daytime - object_creation_datetime_as_object


def send_deletion_request(
    s3_bucket, s3_object_list_to_be_deleted, paging_size=100, logger=None
):
    while s3_object_list_to_be_deleted:
        if logger:
            logger.debug(
                "Objects left in deletion queue: {}".format(
                    len(s3_object_list_to_be_deleted)
                )
            )
        bound = min(len(s3_object_list_to_be_deleted), paging_size)
        if logger:
            logger.debug(
                "Amount of object will be deleted withing subset: {}".format(bound)
            )
        s3_bucket.delete_objects(
            Delete={"Objects": s3_object_list_to_be_deleted[:bound]}
        )
        s3_object_list_to_be_deleted = s3_object_list_to_be_deleted[bound:]
