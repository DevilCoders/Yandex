#!/usr/bin/env python3


from pprint import pprint
import subprocess
import json

from .base import DescriptivePrefixListElement


from cloud.netinfra.rknfilter.yc_rkn_common.manage_configuration_files import read_content_of_suplementary_files
from cloud.netinfra.rknfilter.yc_rkn_common.custom_exceptions import ripe_api_communication_error

# DNS ROOT servers and the other nets
# ASNs for prefixes which should not be blocked


def generate_bgp_asn_whitelist(
    bgp_asn_whitelist_files_locations, predefined_bgp_asn_list=dict(), logger=None
):
    descriptive_bgp_asn_whitelist, plain_bgp_asn_whitelist = generate_network_whitelist(
        populate_prefix_lists_with_prefixes_via_bgp_asn_resolution,
        bgp_asn_whitelist_files_locations,
        default_network_arguments=predefined_bgp_asn_list,
        logger=logger,
    )
    return descriptive_bgp_asn_whitelist, plain_bgp_asn_whitelist


def generate_ip_prefix_whitelist(
    friendly_nets_files_locations, predefined_friendly_net_list=dict(), logger=None
):
    descriptive_ip_prefix_whitelist, plain_ip_prefix_whitelist = generate_network_whitelist(
        populate_prefix_lists_with_prefixes,
        friendly_nets_files_locations,
        default_network_arguments=predefined_friendly_net_list,
        logger=logger
    )
    return descriptive_ip_prefix_whitelist, plain_ip_prefix_whitelist


def generate_network_whitelist(
    whitelist_fulfillment_function,
    files_locations,
    default_network_arguments=dict(),
    logger=None,
):
    plain_ip_prefix_list = set()
    descriptive_ip_prefix_list = set()
    files_content = read_content_of_suplementary_files(files_locations, logger=logger)
    if default_network_arguments:
        files_content.update(default_network_arguments)
    for addressing_block_name, addressing_block_arguments in files_content.items():
        whitelist_fulfillment_function(
            descriptive_ip_prefix_list,
            plain_ip_prefix_list,
            addressing_block_name,
            addressing_block_arguments,
            logger=logger
        )
    return descriptive_ip_prefix_list, plain_ip_prefix_list


def populate_prefix_lists_with_prefixes(
    descriptive_prefix_list,
    plain_prefix_list,
    addressing_block_name,
    addressing_block_arguments,
    addressing_block_charcteristic="MANUALLY DEFINED (NO BGP ASN INFO)",
    logger=None,
):
    if logger:
        logger.debug(
            "Adding prefixes for {} to whitelist".format(addressing_block_name)
        )
    addressing_block_arguments = set(addressing_block_arguments)
    prefix_list_object = DescriptivePrefixListElement(
        addressing_block_charcteristic,
        addressing_block_name,
        addressing_block_arguments,
    )
    descriptive_prefix_list.add(prefix_list_object)
    plain_prefix_list.update(prefix_list_object.ip_prefix_objects)
    if logger:
        logger.debug(
            "Prefixes for {} are added to whitelist".format(addressing_block_name)
        )


def populate_prefix_lists_with_prefixes_via_bgp_asn_resolution(
    descriptive_bgp_asn_prefix_list,
    plain_bgp_asn_prefix_list,
    autonomous_system_name,
    autonomous_system_number,
    logger=None,
):
    if logger:
        logger.debug(
            "Fetching prefixes for : {} ({})".format(
                autonomous_system_name, autonomous_system_number
            )
        )
    bgp_asn_related_prefixes = generate_list_of_prefix_records(
        autonomous_system_number, "white_prefixlist"
    )
    if logger:
        logger.debug("BGP ASN {} is resloved".format(autonomous_system_name))
    populate_prefix_lists_with_prefixes(
        descriptive_bgp_asn_prefix_list,
        plain_bgp_asn_prefix_list,
        autonomous_system_name,
        bgp_asn_related_prefixes,
        autonomous_system_number,
        logger=logger,
    )


def generate_list_of_prefix_records(autonomous_system_number, white_prefix_list_name):
    bgp_asn_related_prefix_records = (
        json.loads(
            fetch_asn_related_prefixes_using_bgpq3(
                autonomous_system_number, white_prefix_list_name
            )
        )
    )[white_prefix_list_name]
    bgp_asn_related_prefixes = [
        bgp_asn_related_prefix_record["prefix"]
        for bgp_asn_related_prefix_record in bgp_asn_related_prefix_records
    ]
    return bgp_asn_related_prefixes


def fetch_asn_related_prefixes_using_bgpq3(
    autonomous_system_number, white_prefix_list_name="raw_bird_formatted_prefix_list"
):
    bgpq3_cli_arguments = [
        "/usr/bin/bgpq3",
        "-h",
        "whois.yandex.net",
        "-l",
        white_prefix_list_name,
        "-R",
        "32",
        "-j",
        autonomous_system_number,
    ]
    try:
        json_with_white_prefix_list = subprocess.check_output(
            " ".join(bgpq3_cli_arguments), shell=True, stderr=subprocess.STDOUT
        ).decode("UTF-8")
    except subprocess.CalledProcessError:
        raise ripe_api_communication_error("Could not fetch info from RIPE server")
    return json_with_white_prefix_list


if __name__ == "__main__":
    WHITE_PREFIX_LIST_NAME = "WhitePrefixList"
    FRIENDLY_NETS = {"google": ["8.8.8.0/24"]}
    WHITELIST_ASNS = {
        #    "Google Inc":"AS15169",
        #    "AWS-EU-AS":"AS9059",
        #    "AMAZON-02":"AS16509",
        #    "Yandex Cloud Technologies LLC": "AS202611",
        #    "YANDEX LLC": "AS13238"
    }
    #    whitelist = generate_whitelist(
    #        WHITELIST_ASNS, FRIENDLY_NETS, WHITE_PREFIX_LIST_NAME
    #    )
    bgp_asn_whitelist_files_locations = {
        "/etc/yc/rkn/white_lists": ["bgp_asn_whitelist.yaml"]
    }
    friendly_nets_files_locations = {"/etc/yc/rkn/white_lists": ["friendly_nets.yaml"]}
    descriptive, plain = generate_ip_prefix_whitelist(
        friendly_nets_files_locations, predefined_friendly_net_list=FRIENDLY_NETS
    )
    for element in descriptive:
        pprint(element.__dict__)
    for element in plain:
        pprint(element)
