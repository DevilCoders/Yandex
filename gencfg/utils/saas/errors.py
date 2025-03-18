# coding: utf-8

from __future__ import absolute_import
from __future__ import division
from __future__ import unicode_literals


class GroupUpdateError(Exception):
    pass


class NotEnoughHosts(GroupUpdateError):
    """
    Not enough hosts for required group configuration
    """
    def __init__(self, required_hosts_count, available_hosts_count, *args, **kwargs):
        super(NotEnoughHosts, self).__init__(*args, **kwargs)
        self.required_hosts = required_hosts_count
        self.available_hosts = available_hosts_count


class ActionNotAllowed(GroupUpdateError):
    """
    Operation require action that was not allowed
    """


class InvalidGroup(GroupUpdateError, ValueError):
    """
    Group do not meet some operation requirements
    """
