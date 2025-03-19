#!/usr/bin/env python3


import boto3
import requests

from botocore.exceptions import ClientError as s3_client_error
from botocore.exceptions import EndpointConnectionError as endpoint_connection_error

from botocore.config import Config


from .s3custom_exceptions import S3connectionError


ENDPOINT_URL = "http://s3.mdst.yandex1.net"
ACCESS_KEY_ID = "BoHLgghFgKtxrFmmWcds"
SECRET_ACCESS_KEY = "8U3JNhy2YZ3K3BYj7K8tZnQxUCsf6vuwzXujtbVE"
S3_BUCKET_NAME = "netinfra-rkn"
WORKING_PREFIX = "Current"
RKN_DUMP_FILE = "dump.xml"
LOCAL_TMP_DIRECTORY = "/tmp/"
BGP_CONFIGURATION_DIRECTORY = "/etc/bird"
BGP_CONFIGURATION_FILE = "bird.conf.static_routes"
SURICATA_CONFIGURATION_DIRECTORY = "/etc/suricata/rules"
SURICATA_CONFIGURATION_FILE = "react.rules"


def get_s3_bucket_context(
    endpoint, access_key_id, secret_access_key, s3_bucket_name, logger=None
):
    if logger:
        logger.info("Organizing connection parameters")
        logger.debug("Creating connection context")
    s3_connection_context = create_s3_connection_context(
        endpoint, access_key_id, secret_access_key, logger
    )
    if logger:
        logger.debug("Creating S3 bucket context")
    s3_bucket_context = create_s3_bucket_context(
        s3_connection_context, s3_bucket_name, logger
    )
    if logger:
        logger.debug("Performing test connection")
    perform_test_connection(s3_bucket_context, endpoint, logger=logger)
    if logger:
        logger.debug("Connection test was succesfull")
    if logger:
        logger.debug("Organizing connection parameters is finished")
    return s3_bucket_context


def create_s3_connection_context(
    endpoint, access_key_id, secret_access_key, logger=None
):
    config = Config(connect_timeout=1, max_pool_connections=1)
    config = Config(connect_timeout=1)
    s3_connnection_context = boto3.resource(
        "s3",
        endpoint_url=endpoint,
        aws_access_key_id=access_key_id,
        aws_secret_access_key=secret_access_key,
        config=config,
    )
    return s3_connnection_context


def create_s3_bucket_context(s3_connection_context, s3_bucket_name, logger=None):
    s3_bucket = s3_connection_context.Bucket(s3_bucket_name)
    return s3_bucket


def perform_test_connection(s3_bucket_context, endpoint, logger=None):
    try:
        list(s3_bucket_context.objects.all())
    except s3_client_error as exception_message:
        logger.critical("core lib exception {}: ".format(exception_message))
        raise S3connectionError(
            "Could not connect to S3 endpoint due to authorization failure: {}\n"
            "Plz check authorization information".format(endpoint)
        )
    except endpoint_connection_error as exception_message:
        logger.critical("core lib exception {}: ".format(exception_message))
        raise S3connectionError(
            "Could not connect to S3 endpoint due to connection reset: {}\n"
            "Plz check endpoint ip/FQDN".format(endpoint)
        )
    except requests.exceptions.ConnectionError as exception_message:
        logger.critical("core lib exception {}: ".format(exception_message))
        raise S3connectionError(
            "Could not connect to S3 endpoint due to connection timeout: {}\n"
            "Plz check network connectivity".format(endpoint)
        )


# if __name__ == "__main__":
#     LOGGER_NAME = __name__
#     LOG_FILE = "/var/log/rkn-S3-operations.log"
#     logger = create_logger_object(LOG_FILE, LOGGER_NAME, False, True)
#     try:
#         s3_bucket = get_s3_bucket_context(
#             ENDPOINT_URL,
#             ACCESS_KEY_ID,
#             SECRET_ACCESS_KEY,
#             S3_BUCKET_NAME,
#             logger=logger,
#         )
#     except S3connectionError as message:
#         logger.critical(message)
