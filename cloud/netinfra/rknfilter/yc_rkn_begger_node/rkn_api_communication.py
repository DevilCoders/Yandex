#! /usr/bin/env python3

import time
import urllib
import suds.client

from datetime import datetime
from collections import defaultdict
from pytz import timezone
from base64 import b64encode, b64decode

from cloud.netinfra.rknfilter.yc_rkn_common.custom_exceptions import rkn_api_communication_error


def create_rkn_soap_api_context(rkn_url, logger=None):
    if logger:
        logger.debug("Creating connection context")
    try:
        connection_context = suds.client.Client(rkn_url)
    except urllib.error.URLError:
        raise rkn_api_communication_error(
            "There was a problem during attempt to create initial connection context"
        )
    if logger:
        logger.debug("Done")
    return connection_context


def check_timestamp_of_last_actual_register_state(
    rkn_soap_api_connection_context, logger=None
):
    rkn_register_timestamps = defaultdict(dict)
    if logger:
        logger.debug("Trying to fetch actual register timestamp from RKN soap api")
    try:
        dates_from_rkn_api = rkn_soap_api_connection_context.service.getLastDumpDateEx()
    except Exception:
        raise rkn_api_communication_error(
            "There was a problem during attempt to retrieve last update status from RKN api"
        )
    if logger:
        logger.debug("Remote api endpoint provided response")
    rkn_register_timestamps["update_timestamp"] = datetime.fromtimestamp(
        dates_from_rkn_api["lastDumpDate"] / 1000, tz=timezone("Europe/Moscow")
    )
    rkn_register_timestamps["urgent_update_timestamp"] = datetime.fromtimestamp(
        dates_from_rkn_api["lastDumpDateUrgently"] / 1000, tz=timezone("Europe/Moscow")
    )
    if logger:
        logger.debug(
            "RKN register update timestamp is: {}".format(
                rkn_register_timestamps["update_timestamp"].isoformat()
            )
        )
        logger.debug(
            "RKN register urgent update timestamp is: {}".format(
                rkn_register_timestamps["urgent_update_timestamp"].isoformat()
            )
        )
    return rkn_register_timestamps


def send_request(
    rkn_soap_api_connection_context,
    cleartext_request_file_path,
    digital_signature_file_path,
    logger=None,
):
    cleartext_request_file_content_base64 = get_aka_byte64_stream_string_from_file(
        cleartext_request_file_path
    )
    digital_signature_file_content_base64 = get_aka_byte64_stream_string_from_file(
        digital_signature_file_path
    )
    remaining_request_attempts = 10
    if logger:
        logger.info("Sending request with digital signature")
    while remaining_request_attempts > 0:
        try:
            response = rkn_soap_api_connection_context.service.sendRequest(
                cleartext_request_file_content_base64,
                digital_signature_file_content_base64,
                "2.2",
            )
        except Exception as exception_msg:
            if logger:
                logger.info(exception_msg)
        if response["result"]:
            request_code = response["code"]
            logger.info("got requested code: {}".format(request_code))
            return request_code
        else:
            remaining_request_attempts -= 1
            logger.debug(
                "Could not get request code. Reason :{}. {} attempts left".format(
                    response["resultComment"], remaining_request_attempts
                )
            )
            time.sleep(3)
    if logger:
        logger.critical("reached max attempts to sendRequest")
        raise rkn_api_communication_error(
            "There was a problem during attempt to retrieve request code"
        )


def get_aka_byte64_stream_string_from_file(file_path):
    with open(file_path, "rb") as file_object:
        file_content_bytestream = file_object.read()
    file_content_bytestream_base64 = b64encode(file_content_bytestream)
    file_content_string_aka_bytestream_base64 = str(
        file_content_bytestream_base64
    ).replace("b'", "")[:-1]
    return file_content_string_aka_bytestream_base64


def retrive_zip_archive_from_rkn_soap_api(
    rkn_soap_api_connection_context, request_code, logger=None
):
    result_code = 0
    remaining_request_attempts = 100
    while remaining_request_attempts > 0:
        try:
            result = rkn_soap_api_connection_context.service.getResult(request_code)
            result_code = result["resultCode"]
            if result_code != 1:
                if logger:
                    logger.debug(
                        "status is: {}, result code: {}. Attempts left: {}".format(
                            result["resultComment"],
                            result_code,
                            remaining_request_attempts,
                        )
                    )
            else:
                zip_archive_file_content = b64decode(result["registerZipArchive"])
                if logger:
                    logger.info(
                        "got zip archive, resultCode: {}".format(result["resultCode"])
                    )
                return zip_archive_file_content
            remaining_request_attempts -= 1
            time.sleep(3)
        except Exception as exc:
            logger.warning("exception appeared: {}".format(exc))
    if logger:
        logger.critical("reached max attempts to getResult")
    raise rkn_api_communication_error(
        "There was a problem during attempt to retrieve zip archive"
    )
