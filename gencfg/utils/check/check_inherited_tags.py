#!/skynet/python/bin/python

"""
    This utility checks if properties of master and slave group match each other. For example, when we have
    property <nonsearch> on master group, we should have the same property on every slave
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict

import gencfg
from core.db import CURDB


def check_slave_and_master_have_same_value(path, master_group, slave_group):
    """
        Function checks if slave and master have same card value at path.

        :type path: list[str]
        :type master_group: core.igroups.IGroup
        :type slave_group: core.igroups.IGroup

        :param path: valid card pth
        :param master_group: master group with slave slave_group
        :param slave_group: slave group with master master_group
        :return (bool, message): pair of (status, message) where status showing if everything is fine and message is error message when status == False
    """

    master_value = master_group.card.get_card_value(path)
    slave_value = slave_group.card.get_card_value(path)

    if slave_value != master_value:
        return False, "Groups <%s> and <%s> have different values <%s> and <%s> in path <%s>" % (master_group.card.name, slave_group.card.name, master_value, slave_value, ".".join(path))
    else:
        return True, None


def main():
    CHECKERS = [
        ('Master and slave have same property nonsearch',
         lambda slave_group, master_group: check_slave_and_master_have_same_value(['properties', 'nonsearch'],
                                                                                  slave_group, master_group)),
        ('Master and slave have same property yasmagent_production_group',
         lambda slave_group, master_group: check_slave_and_master_have_same_value(
             ['properties', 'yasmagent_production_group'], slave_group, master_group)),
        ('Master and slave have same property yasmagent_prestable_group',
         lambda slave_group, master_group: check_slave_and_master_have_same_value(
             ['properties', 'yasmagent_prestable_group'], slave_group, master_group)),
    ]

    failed_checkers = defaultdict(list)
    for checker_name, checker_func in CHECKERS:
        for master_group in filter(lambda x: x.card.master is None, CURDB.groups.get_groups()):
            for slave_group in master_group.slaves:
                status, message = checker_func(master_group, slave_group)
                if not status:
                    failed_checkers[checker_name].append(message)

    if len(failed_checkers) > 0:
        for checker_name in sorted(failed_checkers.keys()):
            print "Checker <%s>:" % checker_name
            for message in failed_checkers[checker_name]:
                print "    %s" % message
        return 1
    else:
        return 0


if __name__ == '__main__':
    status = main()

    sys.exit(int(status))
