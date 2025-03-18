# -*- coding: utf-8 -*-
from library.python.monitoring.solo.objects.solomon.sensor import Sensor


def lf_sensor(**kwargs):
    predefined_labels = dict(project="logfeller")
    for key, value in predefined_labels.items():
        if key not in kwargs:
            kwargs[key] = value
    return Sensor(**kwargs)


def stream_unparsed_sensor(yt_cluster, stream_name):
    return lf_sensor(**{
        "cluster": yt_cluster,
        "service": "indexing",
        "sensor": "parsing",
        "stream-name": stream_name,
        "count": "unparsed-records|splitter-errors"
    })


def reactor_sensor(**kwargs):
    predefined_labels = dict(
        project="reactor",
        cluster="prod",
        service="prod_push",
    )
    for key, value in predefined_labels.items():
        if key not in kwargs:
            kwargs[key] = value
    return Sensor(**kwargs)


# e.g. log_name = "bs-*-log"
def topic_lag_sensor(yt_cluster, log_name):
    return reactor_sensor(
        mode="usertime",
        yt_cluster=yt_cluster,
        sensor="*",
        user_project="logfeller",
        attribute="delay_first_sec",
        account="common_project",
        type="public_artifact",
        time_period="1h",
        log_name=log_name
    )


# e.g. stream_name = "bs-*-log"
def log_ultrafast_failures_count_sensor(yt_cluster, stream_name):
    return reactor_sensor(
        mode="usertime",
        type="reaction",
        sensor="*",
        user_project="logfeller",
        account="common_project",
        yt_cluster=yt_cluster,
        stream_name=stream_name,
        task_type="ultrafast",
        attribute="failures_count",
    )


# e.g. stream_name = "yabs-rt/bs-*-log"
def log_index_failures_count_sensor(yt_cluster, stream_name):
    return reactor_sensor(
        mode="usertime",
        type="reaction",
        sensor="*",
        user_project="logfeller",
        account="common_project",
        yt_cluster=yt_cluster,
        stream_name=stream_name,
        task_type="index_regular",
        attribute="failures_count",
    )


# e.g. log_name = "bs-*-log"
def log_build_failures_count_sensor(yt_cluster, log_name, time_period="*"):
    return reactor_sensor(
        mode="usertime",
        type="reaction",
        sensor="*",
        user_project="logfeller",
        account="common_project",
        yt_cluster=yt_cluster,
        log_name=log_name,
        task_type="build",
        attribute="failures_count",
        time_period=time_period
    )


# e.g. log_name = "bs-*-log"
def log_archive_failures_count_sensor(yt_cluster, log_name, time_period="*"):
    return reactor_sensor(
        mode="usertime",
        type="reaction",
        sensor="*",
        user_project="logfeller",
        account="common_project",
        yt_cluster=yt_cluster,
        log_name=log_name,
        task_type="archive",
        attribute="failures_count",
        time_period=time_period
    )
