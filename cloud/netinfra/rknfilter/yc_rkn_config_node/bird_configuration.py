#!/usr/bin/env python3


def update_bird_configurtion(
    rkn_addressing_db, whitelists, bird_conf_path, logger=None
):
    if logger:
        logger.info("Starting bird configuration")
    with open(bird_conf_path, "w") as bird_conf:
        logger.debug("Opening file {} for writing".format(bird_conf_path))
        define_static_routes_in_bird_configuration_file(
            rkn_addressing_db, bird_conf, logger=logger
        )
        define_whitelist_prefixes_in_bird_configuration_file(
            whitelists, bird_conf, logger=logger
        )
        define_blacklist_prefixes_in_bird_configuration_file(
            rkn_addressing_db, bird_conf, logger=logger
        )
    if logger:
        logger.info("bird configuration finished")


def define_static_routes_in_bird_configuration_file(
    rkn_addressing_db, bird_conf, logger=None
):
    if logger:
        logger.debug("Writing host routes ...")
    bird_conf.write("protocol static rkn_host_routes {\n")
    route_counter = 0
    unique_ip_addresses = sorted(
        set(
            list(rkn_addressing_db.greylist_ip_address_object_set)
            + list(rkn_addressing_db.blacklist_ip_address_object_set)
        )
    )
    print(1)
    for ip_address in unique_ip_addresses:
        route_string = "\troute {} blackhole;\n".format(str(ip_address))
        bird_conf.write(route_string)
        route_counter += 1
    bird_conf.write("}\n\n\n")
    if logger:
        logger.debug("Writing host routes is finished")
    if logger:
        logger.debug("Writing rkn subnet routes ...")
    bird_conf.write("protocol static rkn_subnet_routes {\n")
    unique_ip_prefixes = sorted(
        set(
            list(rkn_addressing_db.greylist_ip_prefix_object_set)
            + list(rkn_addressing_db.blacklist_ip_prefix_object_set)
        )
    )
    for ip_prefix in unique_ip_prefixes:
        route_string = "\troute {} blackhole;\n".format(str(ip_prefix))
        bird_conf.write(route_string)
        route_counter += 1
    if logger:
        logger.debug("Writing rkn subnet routes is finished")
    bird_conf.write("}\n\n\n")
    if logger:
        logger.debug("written rkn routes: {}".format(route_counter))


def define_blacklist_prefixes_in_bird_configuration_file(
    rkn_addressing_db, bird_conf, logger=None
):
    if logger:
        logger.debug("Writing blacklist prefix-list ('function' in Bird Notation ) ...")
    bird_conf.write("function net_blackhole() {\n\treturn ")
    if (
        len(rkn_addressing_db.blacklist_ip_address_object_set) == 0
        and len(rkn_addressing_db.blacklist_ip_prefix_object_set) == 0
    ):
        bird_conf.write("false")
    else:
        bird_conf.write("net ~ [\n")
        write_block_of_bird_function(
            rkn_addressing_db.blacklist_ip_address_object_set,
            bird_conf,
            size_of_remaining_whitelist=len(
                rkn_addressing_db.blacklist_ip_prefix_object_set
            ),
        )
        write_block_of_bird_function(
            rkn_addressing_db.blacklist_ip_prefix_object_set, bird_conf
        )
        bird_conf.write("\t]")
    bird_conf.write(";\n}\n")
    if logger:
        logger.debug(
            "Written black prefix-list entries: {}".format(
                rkn_addressing_db.count_blacklist_routes()
            )
        )


def define_whitelist_prefixes_in_bird_configuration_file(whitelists, bird_conf, logger):
    if logger:
        logger.debug("Writing whitelist prefix-list ('function' in Bird Notation ) ...")
    bird_conf.write("function net_whitelist() {\n\treturn ")
    bgp_asn_ip_prefix_whitelist_size = len(
        whitelists["plain_bgp_asn_ip_prefix_whitelist"]
    )
    bgp_asn_ip_prefix_diamondlist_size = len(
        whitelists["plain_bgp_asn_ip_prefix_diamondlist"]
    )
    friendly_ip_prefix_list_size = len(whitelists["plain_friendly_ip_prefix_list"])
    if (
        bgp_asn_ip_prefix_whitelist_size
        + bgp_asn_ip_prefix_diamondlist_size
        + friendly_ip_prefix_list_size
        == 0
    ):
        bird_conf.write("false")
    else:
        bird_conf.write("net ~ [\n")
        write_block_of_bird_function(
            whitelists["plain_bgp_asn_ip_prefix_whitelist"],
            bird_conf,
            size_of_remaining_whitelist=(
                bgp_asn_ip_prefix_diamondlist_size + friendly_ip_prefix_list_size
            ),
            max_prefix_length=31,
        )
        write_block_of_bird_function(
            whitelists["plain_bgp_asn_ip_prefix_diamondlist"],
            bird_conf,
            size_of_remaining_whitelist=friendly_ip_prefix_list_size,
        )
        write_block_of_bird_function(
            whitelists["plain_friendly_ip_prefix_list"], bird_conf
        )
        bird_conf.write("\t]")
    bird_conf.write(";\n}\n\n\n")
    if logger:
        logger.debug(
            "written whitelist prefix-list entries: {}".format(
                bgp_asn_ip_prefix_whitelist_size
                + bgp_asn_ip_prefix_diamondlist_size
                + friendly_ip_prefix_list_size
            )
        )


def write_block_of_bird_function(
    whitelist, file_object, size_of_remaining_whitelist=0, max_prefix_length=32
):
    whitelist = list(whitelist)
    for net in whitelist:
        if net.prefixlen == 32:
            net_string = "\t\t{}".format(str(net))
        else:
            net_string = "\t\t{}{{{},{}}}".format(
                str(net), net.prefixlen, max_prefix_length
            )
        if net == whitelist[-1] and size_of_remaining_whitelist == 0:
            net_string = net_string + "\n"
        else:
            net_string = net_string + ",\n"
        file_object.write(net_string)
