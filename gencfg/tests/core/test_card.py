import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.card.updater import CardUpdater


def test_update_card_instance_count_func(curdb):
    new_port_func = "old8042"
    updates = {
        ("legacy", "funcs", "instancePort"): "old8042",
    }

    group = curdb.groups.get_group('MAN_WEB_BASE')

    CardUpdater().update_group_card(group, updates)

    assert group.card.legacy.funcs.instancePort == new_port_func
    assert group.get_instances()[0].port == 8042
