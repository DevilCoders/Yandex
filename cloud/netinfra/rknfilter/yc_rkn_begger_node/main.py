#!/usr/bin/env python3

import time
import os

from cloud.netinfra.rknfilter.yc_rkn_s3tools.s3connect import get_s3_bucket_context
from cloud.netinfra.rknfilter.yc_rkn_s3tools.s3put import upload_files_to_s3
from cloud.netinfra.rknfilter.yc_rkn_s3tools.s3custom_exceptions import S3connectionError
from cloud.netinfra.rknfilter.yc_rkn_common.pid_files import create_pid_file, delete_pid_file

from cloud.netinfra.rknfilter.yc_rkn_common.custom_exceptions import (
    input_error_exception,
    nothing_to_do_exception,
    reconfiguration_exception,
    rkn_api_communication_error,
)

from cloud.netinfra.rknfilter.yc_rkn_common.cli_handling import enrich_cli_input
from cloud.netinfra.rknfilter.yc_rkn_common.utils import create_logger_object


from .cli import begger_node_cli
from .rkn_interaction import fetch_rkn_register_dump_file_from_rkn


# CONSTANTS
LOGGER_NAME = __name__

# DEBUG PURPOISE CONSTANTS
TEST_INFO = {
    "s3_bucket_name": "netinfra-rkn",
    "s3_object_key_prefix": "Current",
    "s3_endpoint": "http://s3.mds.yandex.net",
    "s3_auth_key_id": "BoHLgghFgKtxrFmmWcds",
    "s3_secret_auth_key": "8U3JNhy2YZ3K3BYj7K8tZnQxUCsf6vuwzXujtbVE",
    "dump_file_path": "/var/opt/yc/rkn/begger_node/dump.xml",
    "force_downloading_process": False,
    "dump_file_name": "dump.xml",
    "rkn_url": "http://vigruzki.rkn.gov.ru/services/OperatorRequest/?wsdl",
    "operator_name": 'ООО "Облачные Технологии Яндекс"',
    "inn": "7704355605",
    "ogrn": "1167746432040",
    "email": "axscrew@yandex-team.ru",
    "plain_text_request_file_path": "/var/opt/yc/rkn/begger_node/request.xml",
    "digital_signature_file_path": "/var/opt/yc/rkn/begger_node/request.xml.sig",
    "key_file_path": "/var/opt/yc/rkn/begger_node/yandex-telecom-key.pem",
    "certificate_file_path": "/var/opt/yc/rkn/begger_node/yandex-telecom-certificate.pem",
}


def main():
    t0 = time.time()
    cli_input = begger_node_cli()
    if create_pid_file(cli_input.pid_file_path):
        logger = create_logger_object(
            cli_input.logfile, LOGGER_NAME, cli_input.quiet, cli_input.verbose
        )
        try:
            if os.path.isfile("/run/yc-rkn-begger-init.pid"):
                raise reconfiguration_exception(
                    "begger reinitialization is in process"
                )
            cli_input_dict = enrich_cli_input(cli_input, TEST_INFO, logger)
            (
                downloaded_files,
                transaction_id,
            ) = fetch_rkn_register_dump_file_from_rkn(
                cli_input_dict["rkn_url"],
                cli_input_dict["dump_file_path"],
                cli_input_dict["operator_name"],
                cli_input_dict["inn"],
                cli_input_dict["ogrn"],
                cli_input_dict["email"],
                cli_input_dict["plain_text_request_file_path"],
                cli_input_dict["digital_signature_file_path"],
                cli_input_dict["key_file_path"],
                cli_input_dict["certificate_file_path"],
                logger=logger,
            )
            if downloaded_files:
                logger.info("new rkn dump file was successfully fetched")
            else:
                logger.info("rkn dump file is up to date")
            try:
                s3_bucket = get_s3_bucket_context(
                    cli_input_dict["s3_endpoint"],
                    cli_input_dict["s3_auth_key_id"],
                    cli_input_dict["s3_secret_auth_key"],
                    cli_input_dict["s3_bucket_name"],
                    logger=logger,
                )
                upload_files_to_s3(
                    s3_bucket,
                    cli_input_dict["s3_object_key_prefix"],
                    files_locations=[cli_input_dict["dump_file_path"]],
                    transaction_id=transaction_id,
                    logger=logger,
                )
            except S3connectionError as error_message:
                logger.critical(error_message)
                logger.critical("rkn dump file will not be uploaded to S3")
        except input_error_exception as error_message:
            logger.critical(error_message)
            logger.critical("Stopping program execution")
        except S3connectionError as error_message:
            logger.critical(error_message)
            logger.critical("Stopping program execution")
        except reconfiguration_exception as error_message:
            logger.critical(error_message)
            logger.critical("Stopping program execution")
        except rkn_api_communication_error as error_message:
            logger.critical(error_message)
            logger.critical("Stopping program execution")
        except nothing_to_do_exception as error_message:
            logger.info(error_message)
            logger.info("Wait till next time")
        delete_pid_file(cli_input.pid_file_path)
        logger.info("elapsed real time: {}s".format(int(time.time() - t0)))
    else:
        print("Another instance of this service is already running. Stopping")


if __name__ == "__main__":
    main()
