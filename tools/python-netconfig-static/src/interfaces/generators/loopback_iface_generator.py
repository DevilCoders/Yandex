#!/usr/bin/env python

from ..loopback_interface import LoopbackInterface


class LoopbackIfaceGenerator(object):
    def generate(self, parameters, iface_utils, network_info, interfaces):
        if not parameters.no_lo:
            interfaces.add_interface(LoopbackInterface(network_info))
