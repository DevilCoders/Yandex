#!/usr/bin/env python3

from xml.etree import ElementTree

from .base import ContentEntry

from cloud.netinfra.rknfilter.yc_rkn_common.custom_exceptions import configuration_eligibility_exception


def parse_xml_dump(
    files_locations,
    reconfig_triggering_threshholds={"rules_max": 1000000, "rules_min": 100000},
    logger=None,
):
    logger.info("parsing RKN xml dump files...")
    parsed_xml_content = list()
    for file_path in files_locations:
        logger.debug("Parsing file: {}".format(file_path))
        windows_1351_encoding = ElementTree.XMLParser(encoding="cp1251")
        tree = ElementTree.parse(file_path, parser=windows_1351_encoding)
        root = tree.getroot()
        parsed_xml_content += parse_xml(root, reconfig_triggering_threshholds, logger)
        logger.debug("Parsing of file  {} is finished".format(file_path))
    logger.info("parsing RKN xml dumps is  finished")
    return parsed_xml_content


def parse_xml(root, reconfig_triggering_threshholds, logger=None):
    parsed_xml_content = []
    for content_node in root.findall("content"):
        parsed_xml_content.append(parse_single_content_node(content_node))
    if logger:
        amount_of_rkn_content_entries = len(parsed_xml_content)
        logger.info(
            "got rules from dump file: {}".format(amount_of_rkn_content_entries)
        )
    if amount_of_rkn_content_entries > reconfig_triggering_threshholds["rules_max"]:
        raise configuration_eligibility_exception(
            "Too many content entries got from dump, max {}, got: {}, aborted".format(
                reconfig_triggering_threshholds["rules_max"],
                amount_of_rkn_content_entries,
            )
        )
    elif amount_of_rkn_content_entries < reconfig_triggering_threshholds["rules_min"]:
        raise configuration_eligibility_exception(
            "too few urls got from dump, min {}, got: {}, aborted".format(
                reconfig_triggering_threshholds["rules_min"],
                amount_of_rkn_content_entries,
            )
        )
    else:
        if logger:
            logger.debug("Amount of content records within acceptable boundaries")
    return parsed_xml_content


def parse_single_content_node(content_node):
    blocking_type = content_node.get("blockType", default="default")
    rkn_rule_hash = content_node.get("hash")
    content_id = content_node.get("id")
    domains = set()
    ip_prefixes = set()
    ip_addresses = set()
    urls = set()
    for child in content_node:
        if child.tag == "url":
            urls.add(child.text)
        if child.tag == "ip":
            ip_addresses.add(child.text)
        if child.tag == "ipSubnet":
            ip_prefixes.add(child.text)
        if child.tag == "domain":
            domains.add(child.text)
    return ContentEntry(
        content_id,
        rkn_rule_hash,
        ip_addresses,
        ip_prefixes,
        domains,
        urls,
        blocking_type,
    )


if __name__ == "__main__":
    parsed_xml_content = parse_xml_dump(
        "fake_dump.xml", {"rules_max": 1000000, "rules_min": 1}
    )
    print(len(parsed_xml_content))
    for element in parsed_xml_content:
        print(element.__dict__)
