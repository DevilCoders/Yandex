#!/usr/bin/env python3


import time


from .whitelists import generate_whitelists
from .xml_parser import parse_xml_dump
from .normalize_content_element_structure import (
    decompose_parsed_xml_content_into_normalized_rules,
)

from .configuration_engine import generate_configuration_files
from .configuration_engine import perfrom_trivial_check_for_reconfiguration_eligibility

from cloud.netinfra.rknfilter.yc_rkn_s3tools.s3connect import get_s3_bucket_context
from cloud.netinfra.rknfilter.yc_rkn_s3tools.s3get import download_objects_from_s3
from cloud.netinfra.rknfilter.yc_rkn_s3tools.s3put import upload_files_to_s3
from cloud.netinfra.rknfilter.yc_rkn_s3tools.s3delete import delete_objects_from_s3
from cloud.netinfra.rknfilter.yc_rkn_s3tools.s3custom_exceptions import S3connectionError
from cloud.netinfra.rknfilter.yc_rkn_common.pid_files import create_pid_file, delete_pid_file

from .cli import config_node_cli
from cloud.netinfra.rknfilter.yc_rkn_common.cli_handling import enrich_cli_input

from cloud.netinfra.rknfilter.yc_rkn_common.custom_exceptions import (
    input_error_exception,
    configuration_eligibility_exception,
    nothing_to_do_exception,
    ripe_api_communication_error,
    config_node_core_workflow_exception,
)

from cloud.netinfra.rknfilter.yc_rkn_common.local_files import perform_local_copy, archive_files
from cloud.netinfra.rknfilter.yc_rkn_common.utils import create_logger_object


# CONSTANTS
LOGGER_NAME = __name__


# DEBUG PURPOISE CONSTANTS


TEST_INFO = {
    "s3_bucket_name": "netinfra-rkn",
    "s3_object_key_prefix": "Current",
    "s3_endpoint": "http://s3.mds.yandex.net",
    "s3_auth_key_id": "BoHLgghFgKtxrFmmWcds",
    "s3_secret_auth_key": "8U3JNhy2YZ3K3BYj7K8tZnQxUCsf6vuwzXujtbVE",
    "dump_file_path": "/var/opt/yc/rkn/config/dump.xml",
    "bgp_daemon_configuration_file_path": "/var/opt/yc/rkn/config/bird.conf.static_routes",
    "suricata_rules_file_path": "/var/opt/yc/rkn/config/react.rules",
    "bgp_asn_whitelist_files_locations": [
        "/etc/yc/rkn/white_lists/bgp_asn_whitelist.yaml"
    ],
    "bgp_asn_diamondlist_files_locations": [
        "/etc/yc/rkn/white_lists/bgp_asn_diamondlist.yaml"
    ],
    "friendly_nets_files_locations": ["/etc/yc/rkn/white_lists/friendly_nets.yaml"],
    "domain_name_whitelist_files_locations": [
        "/etc/yc/rkn/white_lists/domain_name_whitelist.yaml"
    ],
    "reconfiguration_triggering_threshholds": {  # Crossing of threshhold should stop code execution
        "routes_max": 400000,
        "routes_min": 0,
        "rules_max": 1000000,
        "rules_min": 0,
    },
    "dns_resolve_attempts": 0,
    "use_resolved_addresses_only": False,
    "force_configuration_process": False,
    "s3_object_expiration_days": 14,
    "s3_object_expiration_hours": 0,
    "s3_object_expiration_minutes": 0,
    "fallback_files_locations": [],
}


LOGGER_NAME = __name__


def main():
    cli_input = config_node_cli()
    if create_pid_file(cli_input.pid_file_path):
        t0 = time.time()
        logger = create_logger_object(
            cli_input.logfile, LOGGER_NAME, cli_input.quiet, cli_input.verbose
        )
        try:
            cli_input_dict = enrich_cli_input(cli_input, TEST_INFO, logger)
            if cli_input_dict["dryrun"]:
                if logger:
                    logger.info(
                        "running program in dryrun-mode. S3 communication will be disabled"
                    )
            else:
                dump_files_locations = [cli_input_dict["dump_file_path"]]
                configuration_files_locations = [
                    cli_input_dict["bgp_daemon_configuration_file_path"],
                    cli_input_dict["suricata_rules_file_path"],
                ]
                files_locations = (
                    dump_files_locations + configuration_files_locations
                )
                s3_bucket = get_s3_bucket_context(
                    cli_input_dict["s3_endpoint"],
                    cli_input_dict["s3_auth_key_id"],
                    cli_input_dict["s3_secret_auth_key"],
                    cli_input_dict["s3_bucket_name"],
                    logger=logger,
                )
                downloaded_objects, transaction_id = download_objects_from_s3(
                    s3_bucket,
                    cli_input_dict["s3_object_key_prefix"],
                    files_locations=dump_files_locations,
                    logger=logger,
                )
            if (
                not cli_input_dict["dryrun"]
                and not downloaded_objects
                and not cli_input_dict["force_configuration_process"]
            ):
                raise nothing_to_do_exception(
                    "No files were updated. No reason to start reconfiguration process"
                )
            config_node_core_workflow(cli_input_dict, t0, logger=logger)
            if not cli_input_dict["dryrun"]:
                upload_files_to_s3(
                    s3_bucket,
                    cli_input_dict["s3_object_key_prefix"],
                    files_locations=configuration_files_locations,
                    transaction_id=transaction_id,
                    logger=logger,
                )
                logger.info("Performing backup...")
                archived_files_locations = archive_files(
                    files_locations, logger=logger
                )
                upload_files_to_s3(
                    s3_bucket,
                    transaction_id.strftime("%Y-%m-%d-%H-%M-%S"),
                    files_locations=archived_files_locations,
                    transaction_id=transaction_id,
                    logger=logger,
                )
                logger.info("Backup is done")
                logger.info("Performing old backups rotation")
                delete_objects_from_s3(
                    s3_bucket,
                    s3_object_expiration_days=cli_input_dict[
                        "s3_object_expiration_days"
                    ],
                    s3_object_expiration_hours=cli_input_dict[
                        "s3_object_expiration_hours"
                    ],
                    s3_object_expiration_minutes=cli_input_dict[
                        "s3_object_expiration_minutes"
                    ],
                    logger=logger,
                )
                logger.info("elapsed real time: {}s".format(int(time.time() - t0)))
        except S3connectionError as error_message:
            logger.critical(error_message)
            if cli_input_dict["fallback_files_locations"]:
                logger.info(
                    "Fallback files locations were provided.Trying to get files from local filesystem"
                )
                copied_files = perform_local_copy(
                    cli_input_dict["fallback_files_locations"],
                    dump_files_locations,
                    logger=logger,
                )
                try:
                    if (
                        copied_files
                        or cli_input_dict["force_configuration_process"]
                    ):
                        config_node_core_workflow(cli_input_dict, t0, logger=logger)
                    else:
                        raise nothing_to_do_exception(
                            "No files were updated. No reason to start reconfiguration process"
                        )
                        logger.info(error_message)
                except nothing_to_do_exception as error_message:
                    logger.info(error_message)
                    logger.info("Wait till next time")
                except config_node_core_workflow_exception as error_message:
                    logger.critical(error_message)
                    logger.critical("Stoping program execution")
            else:
                logger.critical("Stopping program execution")
        except input_error_exception as error_message:
            logger.critical(error_message)
            logger.critical("Stopping program execution")
        except config_node_core_workflow_exception as error_message:
            logger.critical(error_message)
            logger.critical("Stoping program execution")
        except nothing_to_do_exception as error_message:
            logger.info(error_message)
            logger.info("Wait till next time")
        logger.debug("time taken : {}s".format(int(time.time() - t0)))
        delete_pid_file(cli_input.pid_file_path)
    else:
        print("Another instance of this service is already running. Stopping")


def config_node_core_workflow(cli_input_dict, t0, logger=None):
    try:
        whitelists = generate_whitelists(
            cli_input_dict["bgp_asn_whitelist_files_locations"],
            cli_input_dict["bgp_asn_diamondlist_files_locations"],
            cli_input_dict["friendly_nets_files_locations"],
            cli_input_dict["domain_name_whitelist_files_locations"],
            logger=logger,
        )
        logger.debug("time taken : {}s".format(int(time.time() - t0)))
        parsed_xml_content = parse_xml_dump(
            [cli_input_dict["dump_file_path"]],
            cli_input_dict["reconfiguration_triggering_threshholds"],
            logger=logger,
        )
        (
            decomposed_rules,
            rkn_addressing_db,
        ) = decompose_parsed_xml_content_into_normalized_rules(
            parsed_xml_content,
            domain_name_whitelist_regex=whitelists["domain_name_whitelist_regex"],
            perform_additional_dns_resolutions=cli_input_dict["dns_resolve_attempts"],
            use_resolved_addresses_only=cli_input_dict["use_resolved_addresses_only"],
            logger=logger,
        )
        perfrom_trivial_check_for_reconfiguration_eligibility(
            rkn_addressing_db,
            whitelists,
            cli_input_dict["reconfiguration_triggering_threshholds"],
            logger=logger,
        )
        logger.debug("time taken : {}s".format(int(time.time() - t0)))
        generate_configuration_files(
            decomposed_rules,
            rkn_addressing_db,
            whitelists,
            cli_input_dict["bgp_daemon_configuration_file_path"],
            cli_input_dict["suricata_rules_file_path"],
            cli_input_dict["reconfiguration_triggering_threshholds"],
            logger=logger,
        )
    except configuration_eligibility_exception as error_message:
        logger.critical(error_message)
        logger.critical("Stopping program execution")
        raise config_node_core_workflow_exception(
            "There was a problem during execution of config node core workflow"
        )
    except ripe_api_communication_error as error_message:
        logger.critical(error_message)
        logger.critical("Stoping program execution")
        raise config_node_core_workflow_exception(
            "There was a problem during execution of config node core workflow"
        )


if __name__ == "__main__":
    main()
