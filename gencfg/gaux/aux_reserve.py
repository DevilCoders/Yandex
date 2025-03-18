#!/skynet/python/bin/python
# coding: utf8

UNKNOW_RESERVE_GROUP = 'ALL_UNSORTED_NEW_HOSTS'
DEFAULT_RESERVE_GROUP = 'RESERVE_NEW_HOSTS'
UNWORKING_NO_MTN_RESERVE_GROUP = 'ALL_UNWORKING_NO_MTN'
TRANSFER_GROUP = 'RESERVE_TRANSFER'
UNSORTED_GROUP = 'ALL_UNSORTED'
PRENALIVKA_GROUP = 'ALL_PRENALIVKA'

WALLE_TAG_TO_RESERVE_GROUP = {
    'qloud': 'ALL_QLOUD_HOSTS',
    'yp': 'ALL_YP_HOSTS',
    'runtime-common': 'ALL_RUNTIME_HOSTS',
}
ALL_PRENALIVKA_TAGS = {'runtime'} | set(WALLE_TAG_TO_RESERVE_GROUP.keys())
ALL_RESERVED_GROUPS = {
    UNKNOW_RESERVE_GROUP,
    DEFAULT_RESERVE_GROUP,
    UNWORKING_NO_MTN_RESERVE_GROUP,
    PRENALIVKA_GROUP
} | set(WALLE_TAG_TO_RESERVE_GROUP.values())


def get_reserve_group(host):
    if not set(host.walle_tags) & ALL_PRENALIVKA_TAGS:
        return UNSORTED_GROUP
    elif 'vlan688' not in host.vlans or 'vlan788' not in host.vlans:
        return UNWORKING_NO_MTN_RESERVE_GROUP

    common_tags = set(host.walle_tags) & set(WALLE_TAG_TO_RESERVE_GROUP.keys())
    if len(common_tags) > 1:
        return UNKNOW_RESERVE_GROUP
    elif len(common_tags) == 0:
        return DEFAULT_RESERVE_GROUP

    walle_tag = list(common_tags)[0]
    return WALLE_TAG_TO_RESERVE_GROUP[walle_tag]


def is_host_transfered(host):
    common_tags = set(host.walle_tags) & ALL_PRENALIVKA_TAGS
    return not common_tags


def move_transfered_hosts(db):
    for host in db.groups.get_group(TRANSFER_GROUP).getHosts():
        if is_host_transfered(host):
            db.groups.move_host(host, UNSORTED_GROUP)


def redistribute_hosts(db):
    for group_name in ALL_RESERVED_GROUPS:
        for host in db.groups.get_group(group_name).getHosts():
            reserved_group_name = get_reserve_group(host)
            if reserved_group_name != group_name:
                db.groups.move_host(host, reserved_group_name)
