#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""helper module to prepare Yandex RFN configs"""


def generate_interface_ip(
    downsream_v4_address,
    ipv4_upstream_prefix="10.255.254.0", ipv4_upstream_prefix_length="24",
    ipv6_upstream_prefix="fe80::cd:c0",  ipv6_upstream_prefix_length="112"
):
    digged_grains= dict()
    last_octet = downsream_v4_address.split(".")[-1]
    usable_ipv4_octets = ipv4_upstream_prefix.split(".")[:-1]
    usable_ipv6_hextets = ipv6_upstream_prefix.split(":")[:-1]
    usable_ipv4_octets.append(last_octet)
    usable_ipv6_hextets.append("C" + last_octet)
    digged_grains["ipv4_upstream_address"] = str(".".join(
        usable_ipv4_octets
    ))
    digged_grains["ipv6_upstream_address"] = str(":".join(
        usable_ipv6_hextets
    ))
    return digged_grains



if __name__ == "__main__":
    print(
        generate_interface_ip("10.12.14.15")
    )

