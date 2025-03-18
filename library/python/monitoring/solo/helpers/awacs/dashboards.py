# -*- coding: utf-8 -*-
from library.python.monitoring.solo.objects.solomon.v2 import Dashboard, Row, Panel, Parameter, Graph, Element, Selector
from library.python.monitoring.solo.util.text import underscore

DEFAULT_REPORT_ID = "service_total"

PARAMETRISE = {
    "host": "{{host}}",
    "cluster": "{{cluster}}",
    "service": "{{service}}",
}


def graph(project_label, **kwargs):
    defaults = dict(
        parameters={
            Parameter(name="project", value=project_label),
            Parameter(name="cluster", value="*"),
            Parameter(name="service", value="*"),
            Parameter(name="host", value="*")
        },
        graph_mode="GRAPH",
        secondary_graph_mode="PIE",
        min="0",
        max="",
        normalize=False,
        color_scheme="AUTO",
        drop_nans=True,
        stack=True,
        aggr="SUM",
        interpolate="LINEAR",
        scale="NATURAL",
        over_lines_transform="NONE",
        ignore_inf=False,
        filter="NONE",
        filter_by="AVG",
        transform="NONE",
        downsampling="AUTO",
        downsampling_aggr="MAX",
        grid="",
        max_points=0,
        hide_no_data=False,
    )
    for key, value in defaults.items():
        if key not in kwargs:
            kwargs[key] = value
    return Graph(**kwargs)


def outgoing_status_code_graph(solomon_project, balancer_id, status_code, report_id, project_label):
    return graph(
        project_label=project_label,
        id="balancer_{0}_outgoing_status_codes_{1}xx".format(underscore(balancer_id), status_code),
        project_id=solomon_project.id,
        name="Balancer {0} Outgoing Status Codes {1}xx".format(balancer_id, status_code),
        elements={
            Element(
                selectors={
                    Selector(name="sensor", value="report-{0}-outgoing_{1}*".format(report_id, status_code)),
                    Selector(name="sensor", value="!report-{0}-outgoing_{1}xx".format(report_id, status_code))
                }
            )
        }
    )


def connections_errors_graph(solomon_project, balancer_id, report_id, project_label):
    return graph(
        project_label=project_label,
        id="balancer_{0}_connections_errors".format(underscore(balancer_id)),
        project_id=solomon_project.id,
        name="Balancer {0} Connections Errors".format(balancer_id),
        elements={
            Element(
                selectors={
                    Selector(name="sensor", value="report-{0}-{1}".format(report_id, sensor))
                }
            ) for sensor in [
                "conn_timeout",
                "conn_fail",
                "conn_refused",
            ]
        }
    )


def backends_errors_graph(solomon_project, balancer_id, report_id, project_label):
    return graph(
        project_label=project_label,
        id="balancer_{0}_backends_errors".format(underscore(balancer_id)),
        project_id=solomon_project.id,
        name="Balancer {0} Backends Errors".format(balancer_id),
        elements={
            Element(
                selectors={
                    Selector(name="sensor", value="report-{0}-{1}".format(report_id, sensor))
                }
            ) for sensor in [
                "backend_error",
                "backend_timeout",
                "backend_fail",
            ]
        }
    )


def general_stats_graph(solomon_project, balancer_id, report_id, project_label):
    return graph(
        project_label=project_label,
        id="balancer_{0}_general_stats".format(underscore(balancer_id)),
        project_id=solomon_project.id,
        name="Balancer {0} General Stats".format(balancer_id),
        elements={
            Element(
                selectors={
                    Selector(name="sensor", value="report-{0}-{1}".format(report_id, sensor))
                }
            ) for sensor in [
                "requests",
                "fail",
                "succ",
            ]
        }
    )


def workers_cpu_graph(solomon_project, balancer_id, report_id, project_label):
    return graph(
        project_label=project_label,
        id="balancer_{0}_workers_cpu".format(underscore(balancer_id)),
        project_id=solomon_project.id,
        name="Balancer {0} Workers CPU".format(balancer_id),
        elements={
            Element(
                type="EXPRESSION",
                expression="""
                    group_by_labels({{"bin"="*", "project"="{0}", "sensor"="worker-cpu_usage"}}, "bin", v -> group_lines("sum", v))
                """.format(project_label),
                title="{{bin}}"
            )
        },
        color_scheme="GRADIENT",
        normalize=True
    )


def processing_time_graph(solomon_project, balancer_id, report_id, project_label):
    return graph(
        project_label=project_label,
        id="balancer_{0}_processing_time".format(underscore(balancer_id)),
        project_id=solomon_project.id,
        name="Balancer {0} Processing Time".format(balancer_id),
        elements={
            Element(
                title="{{bin}}",
                type="EXPRESSION",
                expression="""
                    group_by_labels({{"bin"="*", "project"="{0}", "sensor"="report-{1}-processing_time"}}, "bin", v -> group_lines("sum", v))
                """.format(project_label, report_id)
            )
        },
        over_lines_transform="WEIGHTED_PERCENTILE",
        percentiles="95,97,99,99.5,99.6,99.7"
    )


def generate_row(graphs):
    return Row(
        panels=[
            Panel(
                url=graph.get_dashboard_link(parametrise=PARAMETRISE),
                title=graph.name
            ) for graph in graphs
        ]
    )


def total_dashboard(solomon_project, balancer_id, report_id=None, project_label=None):
    if not project_label:
        project_label = solomon_project.id
    if not report_id:
        report_id = DEFAULT_REPORT_ID

    outgoing_2xx = outgoing_status_code_graph(
        solomon_project, balancer_id, 2, report_id, project_label)
    outgoing_3xx = outgoing_status_code_graph(
        solomon_project, balancer_id, 3, report_id, project_label)
    outgoing_4xx = outgoing_status_code_graph(
        solomon_project, balancer_id, 4, report_id, project_label)
    outgoing_5xx = outgoing_status_code_graph(
        solomon_project, balancer_id, 5, report_id, project_label)

    general_stats = general_stats_graph(solomon_project, balancer_id, report_id, project_label)
    connections_errors = connections_errors_graph(solomon_project, balancer_id, report_id, project_label)
    backends_errors = backends_errors_graph(solomon_project, balancer_id, report_id, project_label)
    workers_cpu = workers_cpu_graph(solomon_project, balancer_id, report_id, project_label)
    processing_time = processing_time_graph(solomon_project, balancer_id, report_id, project_label)

    total_dashboard = Dashboard(
        id="balancer_{0}_total_dashboard".format(underscore(balancer_id)),
        project_id=solomon_project.id,
        name="Balancer {0} Total Dashboard".format(balancer_id),
        parameters={
            Parameter(name="project", value=project_label),
            Parameter(name="cluster", value="*"),
            Parameter(name="service", value="*"),
            Parameter(name="host", value="*")
        },
        rows=[
            generate_row(
                [outgoing_2xx, outgoing_3xx, outgoing_4xx, outgoing_5xx]
            ),
            generate_row(
                [general_stats, connections_errors, backends_errors]
            ),
            generate_row(
                [processing_time, workers_cpu]
            )
        ],
        height_multiplier=1.0
    )

    return [
        outgoing_2xx,
        outgoing_3xx,
        outgoing_4xx,
        outgoing_5xx,
        general_stats,
        connections_errors,
        backends_errors,
        workers_cpu,
        processing_time,
        total_dashboard,
    ]
