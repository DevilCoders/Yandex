#! /usr/bin/env python3


from cloud.netinfra.rknfilter.yc_rkn_common.manage_configuration_files import read_content_of_suplementary_files


def generate_domain_name_whitelist_regex(
    domain_name_whitelist_files_locations,
    default_domain_name_whitelist_patterns=dict(),
    logger=None,
):
    set_of_domain_name_whitelist_patterns = set()
    whitelist_files_content = read_content_of_suplementary_files(
        domain_name_whitelist_files_locations, logger=logger
    )
    whitelist_files_content.update(default_domain_name_whitelist_patterns)
    for domain_name_patterns in whitelist_files_content.values():
        set_of_domain_name_whitelist_patterns.update(domain_name_patterns)
    whitelist_regex = convert_set_of_domain_names_patterns_to_regex(
        set_of_domain_name_whitelist_patterns
    )
    return whitelist_regex


def convert_set_of_domain_names_patterns_to_regex(
    set_of_domain_name_whitelist_patterns, logger=None
):
    regex_elements = list()
    for domain_name_pattern in set_of_domain_name_whitelist_patterns:
        regex_elements.append(convert_domain_name_pattern_to_regex(domain_name_pattern))
    return "|".join(regex_elements)


def convert_domain_name_pattern_to_regex(domain_name_pattern):
    whitelist_regex_element = domain_name_pattern.replace(".", "\\.")
    whitelist_regex_element = whitelist_regex_element.replace("*", ".*")
    return "(^{}$)".format(whitelist_regex_element)


if __name__ == "__main__":
    import re

    white_list_regex = generate_domain_name_whitelist_regex(dict())
    print(white_list_regex)

    for domain_name in [
        "www.google.com",
        "www.google.ru",
        ".yandex.ru",
        "qqq.yandex.ru",
        "www.yandex.com",
        "www.putin-huilo.ru",
        "www.putin-huilo.com",
        "rrr.putin-huilo.com",
        ".putin-huilo.",
    ]:
        print(domain_name)
        print(re.match(white_list_regex, domain_name))
        print("\n")
