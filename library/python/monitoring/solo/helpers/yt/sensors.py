# -*- coding: utf-8 -*-
from library.python.monitoring.solo.objects.solomon.sensor import Sensor


def yt_sensor(**kwargs):
    kwargs.setdefault("project", "yt")
    kwargs.setdefault("service", "accounts")
    return Sensor(**kwargs)


def node_sensor(yt_cluster, account):
    return yt_sensor(
        cluster=yt_cluster,
        account=account,
        sensor="node_count"
    )


def node_limit_sensor(yt_cluster, account):
    return yt_sensor(
        cluster=yt_cluster,
        account=account,
        sensor="node_count_limit"
    )


def chunk_sensor(yt_cluster, account):
    return yt_sensor(
        cluster=yt_cluster,
        account=account,
        sensor="chunk_count"
    )


def chunk_limit_sensor(yt_cluster, account):
    return yt_sensor(
        cluster=yt_cluster,
        account=account,
        sensor="chunk_count_limit"
    )


def tablet_static_memory_sensor(yt_cluster, account):
    return yt_sensor(
        cluster=yt_cluster,
        account=account,
        sensor="tablet_static_memory_in_gb"
    )


def tablet_static_memory_limit_sensor(yt_cluster, account):
    return yt_sensor(
        cluster=yt_cluster,
        account=account,
        sensor="tablet_static_memory_limit_in_gb"
    )


def tablet_count_sensor(yt_cluster, account):
    return yt_sensor(
        cluster=yt_cluster,
        account=account,
        sensor="tablet_count"
    )


def tablet_count_limit_sensor(yt_cluster, account):
    return yt_sensor(
        cluster=yt_cluster,
        account=account,
        sensor="tablet_count_limit"
    )


def disk_space_sensor(yt_cluster, account, medium="default"):
    return yt_sensor(
        cluster=yt_cluster,
        account=account,
        medium=medium,
        sensor="disk_space_in_gb"
    )


def disk_space_limit_sensor(yt_cluster, account, medium="default"):
    return yt_sensor(
        cluster=yt_cluster,
        account=account,
        medium=medium,
        sensor="disk_space_limit_in_gb"
    )
