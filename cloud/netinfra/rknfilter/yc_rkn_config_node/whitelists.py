#! /usr/bin/env python3


from .network_whitelisting import (
    generate_bgp_asn_whitelist,
    generate_ip_prefix_whitelist,
)
from .domain_name_whitelisting import generate_domain_name_whitelist_regex


def generate_whitelists(
    bgp_asn_whitelist_files_locations,
    bgp_asn_diamondlist_files_locations,
    friendly_nets_files_locations,
    domain_name_whitelist_files_locations,
    logger=None,
):
    DEFAULT_DOMAIN_NAME_WHITELIST_PATTERNS = {
        "yandex": ["*.yandex-team.*", "*.yandex.*"],
        "google": ["*.google.*"],
    }

    BGP_ASN_DIAMONDLIST = {
        #    "Google Inc":"AS15169",
        #    "AWS-EU-AS":"AS9059",
        #    "AMAZON-02":"AS16509",
        "Yandex Cloud Technologies LLC": "AS202611",
        "YANDEX LLC": "AS13238"
    }
    DEFAULT_FRIENDLY_NETS = {
        "PREDEFINED_FRIENDLY_NETS": [
            "5.5.5.5/32",
            "8.8.8.8/32", "8.8.4.4/32",
            "198.41.0.4/32", "192.228.79.201/32",
            "192.33.4.12/32", "199.7.91.13/32",
            "192.203.230.10/32", "192.5.5.241/32",
            "192.112.36.4/32", "198.97.190.53/32",
            "192.36.148.17/32", "192.58.128.30",
            "193.0.14.129/32", "199.7.83.42/32",
            "202.12.27.33/32"
        ]
    }
    if logger:
        logger.info("Starting generation of whitelists")
        logger.debug("Starting generation of domain name whitelist")
    domain_name_whitelist_regex = generate_domain_name_whitelist_regex(
        domain_name_whitelist_files_locations,
        default_domain_name_whitelist_patterns=DEFAULT_DOMAIN_NAME_WHITELIST_PATTERNS,
        logger=logger,
    )
    if logger:
        logger.debug("Domain name whitelist generation is finished")
        logger.debug("Starting generation of bgp asn white list")
    descriptive_bgp_asn_ip_prefix_whitelist, plain_bgp_asn_ip_prefix_whitelist = generate_bgp_asn_whitelist(
        bgp_asn_whitelist_files_locations, logger=logger
    )
    if logger:
        logger.debug("BGP ASN whitelist generation is finished")
        logger.debug("Starting generation of bgp asn diamond list")
    descriptive_bgp_asn_ip_prefix_diamondlist, plain_bgp_asn_ip_prefix_diamondlist = generate_bgp_asn_whitelist(
        bgp_asn_diamondlist_files_locations,
        predefined_bgp_asn_list=BGP_ASN_DIAMONDLIST,
        logger=logger
    )
    if logger:
        logger.debug("BGP ASN diamondlist generation is finished")
        logger.debug("Starting generation of friendly nets list")
    descriptive_friendly_ip_prefix_list, plain_friendly_ip_prefix_list = generate_ip_prefix_whitelist(
        friendly_nets_files_locations,
        predefined_friendly_net_list=DEFAULT_FRIENDLY_NETS,
        logger=logger
    )
    if logger:
        logger.debug("Friendly list generation is finished")
        logger.info("Whitelists generation is finished")
    whitelists = {
        "bgp_asn_to_ip_prefix_whitelist": descriptive_bgp_asn_ip_prefix_whitelist,
        "plain_bgp_asn_ip_prefix_whitelist": plain_bgp_asn_ip_prefix_whitelist,
        "descriptive_bgp_asn_ip_prefix_diamondlist": descriptive_bgp_asn_ip_prefix_diamondlist,
        "plain_bgp_asn_ip_prefix_diamondlist": plain_bgp_asn_ip_prefix_diamondlist,
        "descriptive_friendly_ip_prefix_list": descriptive_friendly_ip_prefix_list,
        "plain_friendly_ip_prefix_list": plain_friendly_ip_prefix_list,
        "domain_name_whitelist_regex": domain_name_whitelist_regex,
    }
    return whitelists
