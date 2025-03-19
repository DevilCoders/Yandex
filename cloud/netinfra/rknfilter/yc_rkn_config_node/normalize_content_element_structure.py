#!/usr/bin/env python3

import re

from urllib.parse import urlparse

from .base import DnsCache
from .base import RknAddressingDb
from .base import RknBlockingRule as RBR

from .normalize_url_structure import invert_backslashes

from cloud.netinfra.rknfilter.yc_rkn_common.utils import run_in_parallel


def decompose_parsed_xml_content_into_normalized_rules(
    parsed_xml_content,
    domain_name_whitelist_regex=None,
    perform_additional_dns_resolutions=0,
    use_resolved_addresses_only=False,
    logger=None,
):
    if logger:
        logger.info(
            "decomposing RKN content records to primitive rules of domain-to-ip_address_set/domain-to-ip_prefix_set format \n"
            "and reestimate blocking type if necessery\n"
        )
        if use_resolved_addresses_only:
            logger.info(
                "Only DNS resolved address for L7 rules will be considered for configuration\n"
            )
    decomposed_rules = list()
    if perform_additional_dns_resolutions > 0:
        logger.info(
            "additional DNS resoltiong was requested: {} times".format(
                perform_additional_dns_resolutions
            )
        )
        dns_cache = DnsCache(requests_amount=perform_additional_dns_resolutions)
    else:
        dns_cache = None
        logger.info("additional DNS resolution was not requested")
    rkn_addressing_db = RknAddressingDb()
    run_in_parallel(
        decompose_content_entry,
        parsed_xml_content,
        decomposed_rules=decomposed_rules,
        rkn_addressing_db=rkn_addressing_db,
        dns_cache=dns_cache,
        use_resolved_addresses_only=use_resolved_addresses_only,
        domain_name_whitelist_regex=domain_name_whitelist_regex,
        logger=logger,
    )
    return decomposed_rules, rkn_addressing_db


def decompose_content_entry(
    content_entry,
    decomposed_rules,
    rkn_addressing_db,
    dns_cache,
    domain_name_whitelist_regex,
    use_resolved_addresses_only=False,
):
    try:
        if content_entry.blocking_type == "default":
            decompose_default_btype_content_entry(
                content_entry,
                decomposed_rules,
                rkn_addressing_db,
                dns_cache,
                domain_name_whitelist_regex,
                use_resolved_addresses_only=use_resolved_addresses_only,
            )
        elif content_entry.blocking_type == "domain":
            decompose_domain_btype_content_entry(
                content_entry,
                decomposed_rules,
                rkn_addressing_db,
                dns_cache,
                domain_name_whitelist_regex,
                use_resolved_addresses_only=use_resolved_addresses_only,
            )
        elif content_entry.blocking_type == "domain-mask":
            decompose_domain_mask_btype_content_entry(
                content_entry,
                decomposed_rules,
                rkn_addressing_db,
                domain_name_whitelist_regex,
                use_resolved_addresses_only=use_resolved_addresses_only,
            )
        elif content_entry.blocking_type == "ip":
            decompose_ip_btype_content_entry(
                content_entry, decomposed_rules, rkn_addressing_db
            )
    except Exception as decomposition_exception:
        print(decomposition_exception)


def decompose_ip_btype_content_entry(
    content_entry, decomposed_rules, addressing_database
):
    rbr = RBR(
        content_entry.content_id,
        content_entry.rkn_rule_hash,
        content_entry.blocking_type,
        ip_addresses=content_entry.ip_addresses,
        ip_prefixes=content_entry.ip_prefixes,
    )
    addressing_database.include(rbr)
    decomposed_rules.append(rbr)


def decompose_domain_btype_content_entry(
    content_entry,
    decomposed_rules,
    addressing_database,
    dns_cache=None,
    domain_name_whitelist_regex=None,
    use_resolved_addresses_only=False,
):
    for domain in content_entry.domains:
        if not domain_name_whitelist_regex or not re.match(
            domain_name_whitelist_regex, domain
        ):
            #        if True:
            rbr = RBR(
                content_entry.content_id,
                content_entry.rkn_rule_hash,
                content_entry.blocking_type,
                ip_addresses=content_entry.ip_addresses,
                ip_prefixes=content_entry.ip_prefixes,
                domain=domain,
                url=domain,
                path=domain,
                dns_cache=dns_cache,
            )
            addressing_database.include(rbr, use_resolved_addresses_only)
            decomposed_rules.append(rbr)


def decompose_domain_mask_btype_content_entry(
    content_entry,
    decomposed_rules,
    addressing_database,
    domain_name_whitelist_regex=None,
    use_resolved_addresses_only=False,
):
    for domain in content_entry.domains:
        if not domain_name_whitelist_regex or not re.match(
            domain_name_whitelist_regex, domain
        ):
            #        if True:
            rbr = RBR(
                content_entry.content_id,
                content_entry.rkn_rule_hash,
                content_entry.blocking_type,
                ip_addresses=content_entry.ip_addresses,
                ip_prefixes=content_entry.ip_prefixes,
                domain=domain,
                url=domain,
                path=domain,
            )
            addressing_database.include(rbr, use_resolved_addresses_only)
            decomposed_rules.append(rbr)


def decompose_default_btype_content_entry(
    content_entry,
    decomposed_rules,
    addressing_database,
    dns_cache=None,
    domain_name_whitelist_regex=None,
    use_resolved_addresses_only=False,
):
    if len(content_entry.urls) > 0:
        for url in content_entry.urls:
            url = invert_backslashes(url)
            parsed_url = urlparse(url)
            domain = parsed_url.netloc
            if not domain_name_whitelist_regex or not re.match(
                domain_name_whitelist_regex, domain
            ):
                #            if True:
                path = parsed_url.path
                scheme = parsed_url.scheme
                if len(path) > 1:
                    rbr = RBR(
                        content_entry.content_id,
                        content_entry.rkn_rule_hash,
                        content_entry.blocking_type,
                        ip_addresses=content_entry.ip_addresses,
                        ip_prefixes=content_entry.ip_prefixes,
                        domain=domain,
                        url=url,
                        path=path,
                        scheme=scheme,
                        dns_cache=dns_cache,
                    )
                else:
                    rbr = RBR(
                        content_entry.content_id,
                        content_entry.rkn_rule_hash,
                        "domain",
                        ip_addresses=content_entry.ip_addresses,
                        ip_prefixes=content_entry.ip_prefixes,
                        domain=domain,
                        url=domain,
                        path=path,
                        scheme=scheme,
                        dns_cache=dns_cache,
                    )
                addressing_database.include(rbr, use_resolved_addresses_only)
                decomposed_rules.append(rbr)
    else:
        if content_entry.domains:
            content_entry.blocking_type = "domain"
            decompose_domain_btype_content_entry(
                content_entry,
                decomposed_rules,
                addressing_database,
                dns_cache=dns_cache,
                domain_name_whitelist_regex=domain_name_whitelist_regex,
            )
        else:
            content_entry.blocking_type = "ip"
            decompose_ip_btype_content_entry(
                content_entry, decomposed_rules, addressing_database
            )
