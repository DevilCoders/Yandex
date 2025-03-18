# coding: utf-8

from __future__ import unicode_literals


class Ticket(object):
    @property
    def debug_info(self):
        return self.debug_string


class ServiceTicket(Ticket):
    def __init__(self, ticket_data):
        self.dst = ticket_data['dst']
        self.debug_string = ticket_data['debug_string']
        self.logging_string = ticket_data['logging_string']
        self.scopes = ticket_data['scopes']
        self.src = ticket_data['src']


class UserTicket(Ticket):
    def __init__(self, ticket_data):
        self.debug_string = ticket_data['debug_string']
        self.logging_string = ticket_data['logging_string']
        self.scopes = ticket_data['scopes']
        self.uids = ticket_data['uids']
        self.default_uid = ticket_data['default_uid']
