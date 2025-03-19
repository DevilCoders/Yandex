#! /usr/bin/env python3


import os

from .rkn_request_file import create_cleartext_request_file, create_digital_signature
from .rkn_api_communication import (
    send_request,
    create_rkn_soap_api_context,
    retrive_zip_archive_from_rkn_soap_api,
    check_timestamp_of_last_actual_register_state,
)
from .local_files import (
    check_timestamp_in_dump_file_on_local_filesystem,
    get_register_dump_file_from_zip_archive,
)


def fetch_rkn_register_dump_file_from_rkn(
    rkn_url,
    dump_file_path,
    operator_name,
    inn,
    ogrn,
    email,
    plain_text_request_file_path,
    digital_signature_file_path,
    key_file_path,
    certificate_file_path,
    logger=None,
):
    if logger:
        logger.info("creating connection context")
    rkn_soap_api_connection_context = create_rkn_soap_api_context(
        rkn_url, logger=logger
    )
    rkn_register_timestamps = check_timestamp_of_last_actual_register_state(
        rkn_soap_api_connection_context, logger=logger
    )
    dump_file_timestamps = check_timestamp_in_dump_file_on_local_filesystem(
        dump_file_path, logger=logger
    )
    if logger:
        logger.info("trying to download rkn dump")
    downloaded_files, transaction_id = download_transaction_for_dump_file(
        operator_name,
        inn,
        ogrn,
        email,
        plain_text_request_file_path,
        digital_signature_file_path,
        key_file_path,
        certificate_file_path,
        rkn_soap_api_connection_context,
        dump_file_path,
        rkn_register_timestamps,
        dump_file_timestamps,
        logger=logger,
    )
    return downloaded_files, transaction_id


def download_transaction_for_dump_file(
    operator_name,
    inn,
    ogrn,
    email,
    plain_text_request_file_path,
    digital_signature_file_path,
    key_file_path,
    certificate_file_path,
    rkn_soap_api_connection_context,
    dump_file_path,
    rkn_register_timestamps,
    dump_file_timestamps,
    logger=None,
):
    downloaded_files = set()
    to_download_or_not_download = make_download_decision_for_rkn_dump_file(
        rkn_register_timestamps, dump_file_timestamps, logger=None
    )
    if to_download_or_not_download:
        create_cleartext_request_file(
            operator_name, inn, ogrn, email, plain_text_request_file_path, logger=logger
        )
        create_digital_signature(
            plain_text_request_file_path,
            digital_signature_file_path,
            key_file_path,
            certificate_file_path,
            logger=logger,
        )
        request_code = send_request(
            rkn_soap_api_connection_context,
            plain_text_request_file_path,
            digital_signature_file_path,
            logger=logger,
        )
        zip_archive_file_content = retrive_zip_archive_from_rkn_soap_api(
            rkn_soap_api_connection_context, request_code, logger=logger
        )
        get_register_dump_file_from_zip_archive(
            dump_file_path, zip_archive_file_content, logger=logger
        )
        downloaded_files.add(os.path.basename(dump_file_path))
    transaction_id = max(
        rkn_register_timestamps["update_timestamp"],
        rkn_register_timestamps["urgent_update_timestamp"],
    )
    return downloaded_files, transaction_id


def make_download_decision_for_rkn_dump_file(
    rkn_register_timestamps, dump_files_timestamps, logger=None
):
    to_download_or_not_download = True
    if dump_files_timestamps:
        if (
            rkn_register_timestamps["update_timestamp"]
            == dump_files_timestamps["update_timestamp"]
            and rkn_register_timestamps["urgent_update_timestamp"]
            == dump_files_timestamps["urgent_update_timestamp"]
        ):
            if logger:
                logger.info("Local dump file is up to date")
            to_download_or_not_download = False
        else:
            if logger:
                logger.info("Local file is outdated")
    else:
        if logger:
            logger.info("Local file does not exist")
    return to_download_or_not_download
