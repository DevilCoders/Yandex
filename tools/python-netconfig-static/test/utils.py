import os
import sys


sys.path.append(os.path.join(os.path.dirname(
    os.path.dirname(os.path.realpath(__file__))), 'src'))
from generate_interfaces import build_parser, generate
from functools import partial
from optparse import Values


def get_interfaces(**kwargs):
    parser = build_parser()
    values = parser.defaults.copy()
    values.update(kwargs)
    values = Values(values)
    (opts, args) = parser.parse_args([], values)
    return generate(opts)


def get_interface(name, params):
    """
    Generates interfaces for given parameters and returns interface matching <name>
    Raises AssertionError if the number of matching interfaces is not exactly one

    :param name: interface name to match
    :param params: console options dictionary for interfaces generation
    :return: matching interface
    :rtype interfaces.interface.Interface
    """
    ifaces = [i for i in get_interfaces(**params) if i.name == name]
    assert len(ifaces) == 1
    return ifaces[0]


def _has_matching_action(iface, pattern, action_type=None):
    for action in getattr(iface, action_type):
        if not isinstance(action, basestring):
            action = str(action)
        if pattern in action:
            return True
    return False


has_up = partial(_has_matching_action, action_type='up')
has_postup = partial(_has_matching_action, action_type='postup')
has_postdown = partial(_has_matching_action, action_type='postdown')
