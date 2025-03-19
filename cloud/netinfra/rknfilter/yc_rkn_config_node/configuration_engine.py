#!/usr/bin/env python3


from .bird_configuration import update_bird_configurtion
from .suricata_configuration import update_suricata_configuration
from cloud.netinfra.rknfilter.yc_rkn_common.custom_exceptions import configuration_eligibility_exception


def generate_configuration_files(
    decomposed_rules,
    rkn_addressing_db,
    whitelists,
    bgp_daemon_configuration_file_path,
    suricata_rules_file_path,
    reconfig_triggering_threshholds={"routes_max": 300000, "routes_min": 100000},
    use_resolved_addresses_only=False,
    logger=None,
):
    if logger:
        logger.debug(
            "BGP daemon configuration file will be stored at: {}".format(
                bgp_daemon_configuration_file_path
            )
        )
        logger.debug(
            "Suricata signatures file will be stored at: {}".format(
                suricata_rules_file_path
            )
        )
    if logger:
        logger.info("generating deamons confguration")
    update_bird_configurtion(
        rkn_addressing_db, whitelists, bgp_daemon_configuration_file_path, logger=logger
    )
    update_suricata_configuration(
        decomposed_rules,
        suricata_rules_file_path,
        use_resolved_addresses_only=use_resolved_addresses_only,
        logger=logger,
    )
    if logger:
        logger.info("deamons configuration generation is finished")


def perfrom_trivial_check_for_reconfiguration_eligibility(
    rkn_addressing_db, whitelists, reconfig_triggering_threshholds, logger=None
):
    if logger:
        logger.info("Checking amount of calculeted routes ...")
        logger.debug(
            "There are "
            "{} whitelist prefixes in total".format(
                len(list(whitelists["plain_bgp_asn_ip_prefix_whitelist"]))
                + len(list(whitelists["plain_bgp_asn_ip_prefix_diamondlist"]))
                + len(list(whitelists["plain_friendly_ip_prefix_list"]))
            )
        )
    total_amount_of_routes = rkn_addressing_db.count_host_routes()
    if total_amount_of_routes > reconfig_triggering_threshholds["routes_max"]:
        raise configuration_eligibility_exception(
            "too many routes to install, max {}, got: {}, aborted".format(
                reconfig_triggering_threshholds["routes_max"],
                rkn_addressing_db.count_all_routes(),
            )
        )
    elif total_amount_of_routes < reconfig_triggering_threshholds["routes_min"]:
        raise configuration_eligibility_exception(
            "too few routes to install, min {}, got: {}, aborted".format(
                reconfig_triggering_threshholds["routes_min"],
                rkn_addressing_db.count_all_routes(),
            )
        )
    if logger:
        logger.info(
            "eliginble amount of routes to proceed ({}), continue ...".format(
                total_amount_of_routes
            )
        )
        logger.debug(
            "There are "
            "{} addresses and {} prefixes, with "
            "{} graylist addresses,  {} graylist prefixes "
            "{} blacklist addresses, {} blacklist prefixes".format(
                rkn_addressing_db.count_host_routes(),
                rkn_addressing_db.count_subnet_routes(),
                len(rkn_addressing_db.greylist_ip_address_set),
                len(rkn_addressing_db.greylist_ip_prefix_set),
                len(rkn_addressing_db.blacklist_ip_address_set),
                len(rkn_addressing_db.blacklist_ip_prefix_set),
            )
        )


if __name__ == "__main__":
    pass
