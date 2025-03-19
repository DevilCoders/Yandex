#!/usr/bin/env python3


from .normalize_url_structure import (
    adopt_domain_name_to_suricata_format,
    adopt_url_path_to_suricata_format,
)


def update_suricata_configuration(
    decomposed_rules,
    suricata_rules_filepath,
    use_resolved_addresses_only=False,
    logger=None,
):
    if logger:
        logger.info("updating suricata configuration...")
    last_signature_id = 0
    with open(suricata_rules_filepath, "w") as suricata_rules_file:
        for decomposed_rule in decomposed_rules:
            if use_resolved_addresses_only:
                ip_addresses = decomposed_rule.resolved_ip_addresses
                ip_prefixes = decomposed_rule.resolved_ip_prefixes
            else:
                ip_addresses = decomposed_rule.ip_addresses
                ip_prefixes = decomposed_rule.ip_prefixes
            if ip_addresses or ip_prefixes:
                try:
                    (
                        last_signature_id,
                        suricata_rules_block,
                    ) = convert_decomposed_rule_to_suricata_configuration(
                        last_signature_id, decomposed_rule
                    )
                    suricata_rules_file.write(suricata_rules_block)
                    suricata_rules_file.write("\n")
                except Exception as exception_message:
                    if logger:
                        logger.critical(
                            "could not convert pattern to suricata format. Content id is {}".format(
                                decomposed_rule.content_id
                            )
                        )
                        logger.critical(
                            "excepttion message is: {}".format(exception_message)
                        )
            else:
                if logger:
                    logger.info(
                        "has no addressing information, see rkn_rule_hash: {}".format(
                            decomposed_rule.rkn_rule_hash
                        )
                    )
    if logger:
        logger.info("written suricata rules: {}".format(last_signature_id))


def convert_decomposed_rule_to_suricata_configuration(
    last_signature_id, decomposed_rule
):
    suricata_configuration_block = list()
    if (
        decomposed_rule.blocking_type == "default"
        or decomposed_rule.blocking_type == "domain"
        or decomposed_rule.blocking_type == "domain-mask"
    ):
        domain = adopt_domain_name_to_suricata_format(decomposed_rule)
    if decomposed_rule.blocking_type == "default":
        path = adopt_url_path_to_suricata_format(decomposed_rule)
    for addressing_chunk in split_rule_addressing_to_addressing_chunks(decomposed_rule):
        last_signature_id += 1
        string_with_addresses = generate_string_containing_all_addresses(
            addressing_chunk
        )
        if decomposed_rule.blocking_type == "default":
            suricata_configuration_block.append(
                generate_default_blocking_type_based_suricata_rule(
                    last_signature_id,
                    decomposed_rule,
                    string_with_addresses,
                    domain,
                    path,
                )
            )
        elif decomposed_rule.blocking_type == "domain":
            suricata_configuration_block.append(
                generate_domain_blocking_type_based_suricata_rule(
                    last_signature_id, decomposed_rule, string_with_addresses, domain
                )
            )
        elif decomposed_rule.blocking_type == "domain-mask":
            suricata_configuration_block.append(
                generate_domain_mask_blocking_type_based_suricata_rule(
                    last_signature_id, decomposed_rule, string_with_addresses, domain
                )
            )
        elif decomposed_rule.blocking_type == "ip":
            suricata_configuration_block.append(
                generate_ip_blocking_type_based_suricata_rule(
                    last_signature_id, decomposed_rule, string_with_addresses,
                )
            )
    return last_signature_id, "\n".join(suricata_configuration_block)


def generate_default_blocking_type_based_suricata_rule(
    signature_id, decomposed_rule, string_with_addresses, domain, path
):
    if decomposed_rule.scheme == "http":
        return (
            #            'reject tcp-pkt any any -> {} 80 (msg: "Content_id={}, blocking_type={}"; content:"{}"; nocase; offset:21; content:"{}"; nocase; '
            'reject tcp-pkt any any -> any 80 (msg: "Content_id={}, blocking_type={}"; content:"{}"; nocase; offset:21; content:"{}"; nocase; '
            "offset:3; sid:{}; rev:1;)".format(
                #                string_with_addresses,
                decomposed_rule.content_id,
                decomposed_rule.blocking_type,
                domain,
                path,
                signature_id,
            )
        )
    elif decomposed_rule.scheme == "https":
        return (
            #            'reject tcp-pkt any any -> {} 443  (msg: "Content_id={}, blocking_type={}"; content:"{}"; nocase; sid:{}; rev:1;)'.format(
            'reject tcp-pkt any any -> any 443  (msg: "Content_id={}, blocking_type={}"; content:"{}"; nocase; sid:{}; rev:1;)'.format(
                #                string_with_addresses,
                decomposed_rule.content_id,
                decomposed_rule.blocking_type,
                domain,
                signature_id,
            )
        )


def generate_domain_blocking_type_based_suricata_rule(
    signature_id, decomposed_rule, string_with_addresses, domain
):
    if decomposed_rule.scheme == "http":
        return (
            #            'reject tcp-pkt any any -> {} 80 (msg: "Content_id={}, blocking_type={}"; content:"{}"; nocase; sid:{}; rev:1;)'.format(
            'reject tcp-pkt any any -> any 80 (msg: "Content_id={}, blocking_type={}"; content:"{}"; nocase; sid:{}; rev:1;)'.format(
                #                string_with_addresses,
                decomposed_rule.content_id,
                decomposed_rule.blocking_type,
                domain,
                signature_id,
            )
        )
    elif decomposed_rule.scheme == "https":
        return (
            #            'reject tcp-pkt any any -> {} 443 (msg: "Content_id={}, blocking_type={}"; content:"{}"; nocase; sid:{}; rev:1;)'.format(
            'reject tcp-pkt any any -> any 443 (msg: "Content_id={}, blocking_type={}"; content:"{}"; nocase; sid:{}; rev:1;)'.format(
                #                string_with_addresses,
                decomposed_rule.content_id,
                decomposed_rule.blocking_type,
                domain,
                signature_id,
            )
        )
    elif decomposed_rule.scheme == "both":
        return (
            #            'reject tcp-pkt any any -> {} [80, 443] (msg: "Content_id={}, blocking_type={}"; content:"{}"; nocase; sid:{}; rev:1;)'.format(
            'reject tcp-pkt any any -> any [80, 443] (msg: "Content_id={}, blocking_type={}"; content:"{}"; nocase; sid:{}; rev:1;)'.format(
                #                string_with_addresses,
                decomposed_rule.content_id,
                decomposed_rule.blocking_type,
                domain,
                signature_id,
            )
        )
    else:
        return "#something strange with {}".format(decomposed_rule.content_id)


def generate_domain_mask_blocking_type_based_suricata_rule(
    signature_id, decomposed_rule, string_with_addresses, domain
):
    return (
        #        'reject tcp-pkt any any -> {} [80,443] (msg: "Content_id={}, blocking_type={}"; content:"{}"; nocase; sid:{}; rev:1;)'.format(
        'reject tcp-pkt any any -> any [80,443] (msg: "Content_id={}, blocking_type={}"; content:"{}"; nocase; sid:{}; rev:1;)'.format(
            #            string_with_addresses,
            decomposed_rule.content_id,
            decomposed_rule.blocking_type,
            domain,
            signature_id,
        )
    )


def generate_ip_blocking_type_based_suricata_rule(
    signature_id, decomposed_rule, string_with_addresses
):
    return (
        #        'reject tcp-pkt any any -> {} any (msg: "Content_id={}, blocking_type={}"; pcre:"/.*/i"; sid:{}; rev:1;)'.format(
        'reject tcp-pkt any any -> {} any (msg: "Content_id={}, blocking_type={}"; pcre:"/./i"; sid:{}; rev:1;)'.format(
            string_with_addresses,
            decomposed_rule.content_id,
            decomposed_rule.blocking_type,
            signature_id,
        )
    )


def generate_string_containing_all_addresses(addressing_chunk):
    return (
        "["
        + ",".join(addressing_chunk["ip_addresses"] + addressing_chunk["ip_prefixes"])
        + "]"
    )


# def adopt_domain_name_to_suricata_format(string, pcre=False):
#    if pcre:
#        new_string = ".*" + string.split("*")[-1].replace(".","\.")
#        return new_string
#    else:
#        #new_string = string.split("|")[0].split(";")[0].replace("\\","/")
#        new_string = string.split("|")[0].split(";")[0].replace("\\","/").replace("*.","")
#        if len(new_string) > 255:
#            return new_string[:255]
#        else:
#            return new_string


def split_rule_addressing_to_addressing_chunks(decomposed_rule, block_size=250):
    ip_addresses = list(decomposed_rule.ip_addresses)
    ip_prefixes = list(decomposed_rule.ip_prefixes)
    addressing_chunks = list()
    block_number = 0
    block_starting_element = 0
    while block_size * (block_number + 1) + block_starting_element <= len(ip_addresses):
        addressing_chunks.append(
            {
                "ip_addresses": ip_addresses[
                    block_size * block_number
                    + block_starting_element: block_size * (block_number + 1)
                    + block_starting_element
                ],
                "ip_prefixes": [],
            }
        )
        block_number += 1
    addressing_chunks.append(
        {
            "ip_addresses": ip_addresses[
                block_size * block_number + block_starting_element:
            ],
            "ip_prefixes": [],
        }
    )
    block_starting_element = block_size - len(addressing_chunks[-1]["ip_addresses"])
    block_number = 0
    if len(ip_prefixes) <= block_starting_element:
        addressing_chunks[-1]["ip_prefixes"] = ip_prefixes
    else:
        addressing_chunks[-1]["ip_prefixes"] = ip_prefixes[:block_starting_element]
        while block_size * (block_number + 1) + block_starting_element <= len(
            ip_prefixes
        ):
            addressing_chunks.append(
                {
                    "ip_addresses": [],
                    "ip_prefixes": ip_prefixes[
                        block_size * block_number
                        + block_starting_element: block_size * (block_number + 1)
                        + block_starting_element
                    ],
                }
            )
            block_number += 1
        addressing_chunks.append(
            {
                "ip_addresses": [],
                "ip_prefixes": ip_prefixes[
                    block_size * block_number + block_starting_element:
                ],
            }
        )
    return addressing_chunks
