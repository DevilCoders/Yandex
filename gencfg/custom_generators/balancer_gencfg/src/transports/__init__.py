# -*- coding: utf-8 -*-
"""
"""
from base_db_transport import InstanceDbTransport


class InstanceDbTransportHolder(object):
    _transport_instance = InstanceDbTransport()

    @staticmethod
    def get_transport():
        return InstanceDbTransportHolder._transport_instance

    @staticmethod
    def set_transport(transport):
        InstanceDbTransportHolder._transport_instance = transport
