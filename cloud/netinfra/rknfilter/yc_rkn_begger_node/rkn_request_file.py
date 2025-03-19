#!/usr/bin/env python3

import os
import time
import subprocess

from cloud.netinfra.rknfilter.yc_rkn_common.custom_exceptions import reconfiguration_exception


def create_cleartext_request_file(
    operator_name, inn, ogrn, email, cleartext_request_file_path, logger=None
):
    if logger:
        logger.debug("creating content of request file")
    check_directory_for_target_file_exists(
        "Request file", cleartext_request_file_path, logger=logger
    )
    request_file_content = (
        '<?xml version="1.0" encoding="windows-1251"?>'
        "<request>"
        "<requestTime>{}</requestTime>"
        "<operatorName>{}</operatorName>"
        "<inn>{}</inn>"
        "<ogrn>{}</ogrn>"
        "<email>{}</email>"
        "</request>"
    ).format(
        time.strftime("%Y-%m-%dT%H:%M:%S.000+03:00"),
        #        datetime.now(tz=timezone("Europe/Moscow")).isoformat(),
        operator_name,
        inn,
        ogrn,
        email,
    )
    logger.debug("request file content:\n{}".format(request_file_content))
    with open(
        cleartext_request_file_path, "w+", encoding="cp1251"
    ) as cleartext_request_file:
        cleartext_request_file.write(request_file_content)
    if logger:
        logger.info("request file created")


def create_digital_signature(
    cleartext_request_file_path,
    digital_signature_file_path,
    key_file_path,
    certificate_file_path,
    logger=None,
):
    if logger:
        logger.info("Prepare to sign request file")
    shell_command = (
        "/usr/bin/docker run"
        " -v {}:/certificate.pem"
        " -v {}:/key.pem"
        " -v {}:/request"
        " -v {}:/digital_signature"
        " -t docker-gost"
        " /usr/local/ssl/bin/openssl cms"
        " -signer /certificate.pem"
        " -in /request -sign"
        " -inkey /key.pem"
        " -outform DER"
        " -out /digital_signature".format(
            certificate_file_path,
            key_file_path,
            cleartext_request_file_path,
            digital_signature_file_path,
        )
    )
    check_if_file_exists("Certificate file", certificate_file_path, logger=logger)
    check_if_file_exists("Key file", key_file_path, logger=logger)
    if os.path.isfile(digital_signature_file_path):
        if logger:
            logger.debug(
                "Old signature file already exists. Going to overwrite it: {}".format(
                    digital_signature_file_path
                )
            )
    else:
        check_directory_for_target_file_exists(
            "Digital Signature file", digital_signature_file_path, logger=logger
        )
        logger.debug("Creating blank file: {}".format(digital_signature_file_path))
        with open(digital_signature_file_path, "w") as blank_file_object:
            blank_file_object.write("")
    try:
        logger.info("issuing digital signature... ")
        logger.debug("executing: {}".format(shell_command))
        shell_command_output = subprocess.check_output(
            shell_command, shell=True, stderr=subprocess.STDOUT
        )
        logger.debug("got output: {}".format(shell_command_output))
    except subprocess.CalledProcessError as exc:
        logger.critical("{} ReturnCode: {}, aborted".format(exc.output, exc.returncode))
        raise reconfiguration_exception(
            "There was a problem during issuing digital signature"
        )
    if logger:
        logger.info(
            "Got digital signature file: {}".format(digital_signature_file_path)
        )


def check_if_file_exists(file_type, file_path, logger=None):
    if os.path.isfile(file_path):
        if logger:
            logger.debug("{} file is found {}".format(file_type, file_path))
    else:
        raise reconfiguration_exception(
            "{} file was not found: {}".format(file_type, file_path)
        )


def check_directory_for_target_file_exists(file_type, file_path, logger=None):
    if not os.path.exists(os.path.dirname(file_path)):
        if logger:
            logger.debug(
                "Directory for storing {} does not exist. Creating it: {}".format(
                    file_type, os.path.dirname(file_path)
                )
            )
        os.makedirs(os.path.dirname(file_path))
    else:
        logger.debug("Directory for storing {} file already exists".format(file_type))
