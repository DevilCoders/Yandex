# -*- coding: utf-8 -*-
from datetime import timedelta

from library.python.monitoring.solo.helpers.logfeller.sensors import stream_unparsed_sensor, \
    log_ultrafast_failures_count_sensor, log_archive_failures_count_sensor, log_build_failures_count_sensor, log_index_failures_count_sensor
from library.python.monitoring.solo.objects.solomon.v2 import MultiAlert, Type, Expression

# TODO: add lag alerts from yabs/solo_yabs


def lf_unparsed_multialert(solomon_project, yt_cluster, stream_name_filter, tag, stream_name_neq=None):
    sensor = str(stream_unparsed_sensor(yt_cluster, stream_name_filter))
    if stream_name_neq:
        # nasty =(
        sensor = sensor.replace("\"stream-name\"=", "\"stream-name\"!=\"{0}\", \"stream-name\"=".format(stream_name_neq))

    return MultiAlert(
        id="lf_{0}_{1}_unparsed".format(yt_cluster, tag),
        project_id=solomon_project.id,
        name="Logfeller: {0} {1} unparsed".format(yt_cluster, tag),
        type=Type(
            expression=Expression(
                program="""
                    let unparsed = group_lines("sum", {sensor});
                    alarm_if(max(unparsed) > 0);
                """.format(sensor=sensor)
            )
        ),
        group_by_labels={
            "stream-name",
            "time-period"
        },
        window_secs=int(timedelta(hours=12).total_seconds()),
    )


def lf_ultrafast_fails_multialert(solomon_project, yt_cluster, stream_name_filter, tag):
    return MultiAlert(
        id="lf_{0}_{1}_ultrafast_fails".format(yt_cluster, tag),
        project_id=solomon_project.id,
        name="Logfeller: {0} {1} ultrafast parsing fails".format(yt_cluster, tag),
        type=Type(
            expression=Expression(
                program="""
                    let failures = group_lines("sum", {sensor});
                    let last_points = tail(failures, 10);
                    alarm_if(sum(last_points) > 4);
                """.format(sensor=log_ultrafast_failures_count_sensor(
                    yt_cluster=yt_cluster, stream_name=stream_name_filter
                ))
            )
        ),
        group_by_labels={
            "stream_name"
        },
        window_secs=int(timedelta(hours=12).total_seconds()),
    )


def lf_index_fails_multialert(solomon_project, yt_cluster, stream_name_filter, tag):
    return MultiAlert(
        id="lf_{0}_{1}_index_fails".format(yt_cluster, tag),
        project_id=solomon_project.id,
        name="Logfeller: {0} {1} indexing fails".format(yt_cluster, tag),
        type=Type(
            expression=Expression(
                program="""
                    let failures = group_lines("sum", {sensor});
                    let last_points = tail(failures, 10);
                    alarm_if(sum(last_points) > 4);
                """.format(sensor=log_index_failures_count_sensor(
                    yt_cluster=yt_cluster, stream_name=stream_name_filter
                ))
            )
        ),
        group_by_labels={
            "stream_name"
        },
        window_secs=int(timedelta(hours=12).total_seconds()),
    )


def lf_build_fails_multialert(solomon_project, yt_cluster, log_name_filter, tag):
    return MultiAlert(
        id="lf_{0}_{1}_build_fails".format(yt_cluster, tag),
        project_id=solomon_project.id,
        name="Logfeller: {0} {1} build fails".format(yt_cluster, tag),
        type=Type(
            expression=Expression(
                program="""
                    let failures = group_lines("sum", {sensor});
                    let last_points = tail(failures, 5);
                    alarm_if(sum(last_points) > 2);
                """.format(sensor=log_build_failures_count_sensor(
                    yt_cluster=yt_cluster, log_name=log_name_filter
                ))
            )
        ),
        group_by_labels={
            "log_name",
            "time_period"
        },
        window_secs=int(timedelta(days=6).total_seconds()),
    )


def lf_archive_fails_multialert(solomon_project, yt_cluster, log_name_filter,  tag):
    return MultiAlert(
        id="lf_{0}_{1}_archive_fails".format(yt_cluster, tag),
        project_id=solomon_project.id,
        name="Logfeller: {0} {1} archive fails".format(yt_cluster, tag),
        type=Type(
            expression=Expression(
                program="""
                    let failures = group_lines("sum", {sensor});
                    let last_points = tail(failures, 5);
                    alarm_if(sum(last_points) > 2);
                """.format(sensor=log_archive_failures_count_sensor(
                    yt_cluster=yt_cluster, log_name=log_name_filter
                ))
            )
        ),
        group_by_labels={
            "log_name",
            "time_period"
        },
        window_secs=int(timedelta(days=6).total_seconds()),
    )
