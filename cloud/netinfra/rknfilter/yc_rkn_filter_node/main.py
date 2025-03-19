#!/usr/bin/env python3

from cloud.netinfra.rknfilter.yc_rkn_s3tools.s3connect import get_s3_bucket_context
from cloud.netinfra.rknfilter.yc_rkn_s3tools.s3get import download_objects_from_s3
from cloud.netinfra.rknfilter.yc_rkn_s3tools.s3custom_exceptions import S3connectionError

from cloud.netinfra.rknfilter.yc_rkn_common.custom_exceptions import (
    input_error_exception,
    nothing_to_do_exception,
    reconfiguration_exception,
)
from cloud.netinfra.rknfilter.yc_rkn_common.local_files import perform_local_copy
from cloud.netinfra.rknfilter.yc_rkn_common.utils import create_logger_object
from cloud.netinfra.rknfilter.yc_rkn_common.pid_files import create_pid_file, delete_pid_file
from cloud.netinfra.rknfilter.yc_rkn_common.cli_handling import enrich_cli_input

from .cli import filter_node_cli
from .demons import apply_new_configuration


# CONSTANTS
LOGGER_NAME = __name__

# DEBUG PURPOISE CONSTANTS


TEST_INFO = {
    "s3_bucket_name": "netinfra-rkn",
    "s3_object_key_prefix": "Current",
    "s3_endpoint": "http://s3.mds.yandex.net",
    "s3_auth_key_id": "BoHLgghFgKtxrFmmWcds",
    "s3_secret_auth_key": "8U3JNhy2YZ3K3BYj7K8tZnQxUCsf6vuwzXujtbVE",
    "bgp_daemon_configuration_file_path": "/etc/bird/bird.conf.static_routes",
    "suricata_rules_file_path": "/etc/suricata/rules/react.rules",
    "fallback_files_locations": [],
}


def main():
    cli_input = filter_node_cli()
    if create_pid_file(cli_input.pid_file_path):
        logger = create_logger_object(
            cli_input.logfile, LOGGER_NAME, cli_input.quiet, cli_input.verbose
        )
        try:
            cli_input_dict = enrich_cli_input(cli_input, TEST_INFO, logger)
            configuration_files_locations = [
                cli_input_dict["bgp_daemon_configuration_file_path"],
                cli_input_dict["suricata_rules_file_path"],
            ]
            s3_bucket = get_s3_bucket_context(
                cli_input_dict["s3_endpoint"],
                cli_input_dict["s3_auth_key_id"],
                cli_input_dict["s3_secret_auth_key"],
                cli_input_dict["s3_bucket_name"],
                logger=logger,
            )
            downloaded_objects, _ = download_objects_from_s3(
                s3_bucket,
                cli_input_dict["s3_object_key_prefix"],
                files_locations=configuration_files_locations,
                logger=logger,
            )
            if not downloaded_objects:
                raise nothing_to_do_exception(
                    "No files were updated. No reason to start reconfiguration process"
                )
            apply_new_configuration(logger=logger)
        except S3connectionError as error_message:
            logger.critical(error_message)
            if cli_input_dict["fallback_files_locations"]:
                logger.info(
                    "Fallback files locations were provided.Trying to get files from local filesystem"
                )
                copied_files = perform_local_copy(
                    cli_input_dict["fallback_files_locations"],
                    configuration_files_locations,
                    logger=logger,
                )
                try:
                    if copied_files:
                        apply_new_configuration(logger=logger)
                    else:
                        raise nothing_to_do_exception(
                            "No files were updated. No reason to start reconfiguration process"
                        )
                except reconfiguration_exception as error_message:
                    logger.critical(error_message)
                    logger.critical("Stopping program execution")
                except reconfiguration_exception as error_message:
                    logger.critical(error_message)
                    logger.critical("Stopping program execution")
        except input_error_exception as error_message:
            logger.critical(error_message)
            logger.critical("Stopping program execution")
        except nothing_to_do_exception as error_message:
            logger.info(error_message)
            logger.info("Wait till next time")
        except reconfiguration_exception as error_message:
            logger.critical(error_message)
            logger.critical("Stopping program execution")
        delete_pid_file(cli_input.pid_file_path)
    else:
        print("Another instance of this service is already running. Stopping")


if __name__ == "__main__":
    main()
