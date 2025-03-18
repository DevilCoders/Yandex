# -*- coding: utf-8 -*-
import jsonobject
import six

try:
    from urllib import urlencode
except ImportError:
    from urllib.parse import urlencode

from library.python.monitoring.solo.objects.solomon.v2.base import SolomonObject


class SolomonCluster:
    STABLE = "https://solomon.yandex-team.ru"  # TODO: move to global module
    PRESTABLE = "http://solomon-prestable.yandex.net"
    TESTING = "http://solomon-test.yandex.net"


class Selector(SolomonObject):
    name = jsonobject.StringProperty(default="")
    value = jsonobject.StringProperty(default="")


class Parameter(SolomonObject):
    name = jsonobject.StringProperty(default="")
    value = jsonobject.StringProperty(default="")


class Element(SolomonObject):
    title = jsonobject.StringProperty(default="")
    type = jsonobject.StringProperty(default="SELECTORS")
    selectors = jsonobject.SetProperty(Selector)
    expression = jsonobject.StringProperty(default="")
    link = jsonobject.StringProperty(default="")
    area = jsonobject.BooleanProperty(default=True)
    stack = jsonobject.StringProperty(default="")
    down = jsonobject.BooleanProperty(exclude_if_none=True)
    color = jsonobject.StringProperty(default="")
    yaxis = jsonobject.StringProperty(default="LEFT")
    transform = jsonobject.StringProperty(default="NONE")


class Graph(SolomonObject):
    __OBJECT_TYPE__ = "Graph"

    id = jsonobject.StringProperty(default="")
    project_id = jsonobject.StringProperty(name="projectId")
    name = jsonobject.StringProperty(default="")
    description = jsonobject.StringProperty(default="")

    parameters = jsonobject.SetProperty(Parameter)
    elements = jsonobject.SetProperty(Element)

    # Display tab
    graph_mode = jsonobject.StringProperty(name="graphMode",
                                           choices=["GRAPH", "PIE", "BARS", "DISTRIBUTION", "HEATMAP", "NONE"],
                                           default="GRAPH")
    secondary_graph_mode = jsonobject.StringProperty(name="secondaryGraphMode",
                                                     choices=["PIE", "BARS", "DISTRIBUTION", "HEATMAP", "NONE"],
                                                     default="PIE")
    # graphMode: GRAPH
    stack = jsonobject.BooleanProperty(default=None)
    normalize = jsonobject.BooleanProperty()
    # graphMode: PIE
    color_scheme = jsonobject.StringProperty(name="colorScheme", choices=["AUTO", "DEFAULT", "GRADIENT"],
                                             default="AUTO")
    green = jsonobject.StringProperty(default="")
    yellow = jsonobject.StringProperty(default="")
    red = jsonobject.StringProperty(default="")
    violet = jsonobject.StringProperty(default="")
    # graphMode: BARS, Y-axis (UI).
    scale = jsonobject.StringProperty(choices=["NATURAL", "LOG"])
    # Y-axis (UI). example: "numberFormat": "3|G"
    number_format = jsonobject.StringProperty(name="numberFormat", default="")
    # graphMode: HEATMAP. Heatmap thresholds
    green_value = jsonobject.StringProperty(name="greenValue", default="")
    yellow_value = jsonobject.StringProperty(name="yellowValue", default="")
    red_value = jsonobject.StringProperty(name="redValue", default="")
    violet_value = jsonobject.StringProperty(name="violetValue", default="")
    # Others
    interpolate = jsonobject.StringProperty(choices=["LINEAR", "LEFT", "RIGHT", "NONE"], default="LINEAR")
    drop_nans = jsonobject.BooleanProperty(name="dropNans", default=None)
    aggr = jsonobject.StringProperty(choices=["AVG", "MIN", "MAX", "LAST", "SUM"], default="AVG")

    # Transformation tab
    transform = jsonobject.StringProperty(
        choices=["DIFFERENTIATE", "DIFFERENTIATE_WITH_NEGATIVE", "INTEGRATE", "MOVING_AVERAGE", "MOVING_PERCENTILE",
                 "DIFF", "NONE"], default="DIFFERENTIATE")
    over_lines_transform = jsonobject.StringProperty(name="overLinesTransform",
                                                     choices=["NONE", "PERCENTILE", "WEIGHTED_PERCENTILE", "SUMMARY"])
    # overLinesTransform: PERCENTILE
    percentiles = jsonobject.StringProperty(default="")
    ignore_inf = jsonobject.BooleanProperty(name="ignoreInf")
    # Filter
    filter = jsonobject.StringProperty(choices=["NONE", "TOP", "BOTTOM"], default="NONE")
    filter_by = jsonobject.StringProperty(name="filterBy", choices=["AVG", "MIN", "MAX", "LAST", "SUM"], default="AVG")
    filter_limit = jsonobject.StringProperty(name="filterLimit", default="")
    # Downsampling
    downsampling = jsonobject.StringProperty(choices=["AUTO", "BY_INTERVAL", "BY_POINTS", "OFF"], default="BY_INTERVAL")
    downsampling_aggr = jsonobject.StringProperty(name="downsamplingAggr", choices=["DEFAULT", "AVG", "MIN", "MAX", "LAST", "SUM"],
                                                  default="AVG")
    grid = jsonobject.StringProperty(default="")
    # Others
    limit = jsonobject.StringProperty(default="")

    min = jsonobject.StringProperty(default="")
    max = jsonobject.StringProperty(default="")

    moving_window = jsonobject.StringProperty(name="movingWindow", default="")
    moving_percentile = jsonobject.StringProperty(name="movingPercentile", default="")
    max_points = jsonobject.IntegerProperty(name="maxPoints")
    hide_no_data = jsonobject.BooleanProperty(name="hideNoData")
    threshold = jsonobject.FloatProperty(default=None)
    version = jsonobject.IntegerProperty(name="version", required=True, default=0)

    def get_link(self, use_legend=True, urlencoded=True, parametrise=None):
        """
        Returns link without server name
        :param use_legend: show legend on graph or not
        :param parametrise: dict[str, str]
        parametrise can be use in dashboards for setting parameters:
        for example, parametrise = {"environment": "{{environment}}"}
        will use "environment" label value from dashboard parameters
        :return:
        """
        details = {p.name: p.value for p in self.parameters}
        details.update({
            "project": self.project_id,
            "graph": self.id,
        })
        if use_legend is not None:
            details["legend"] = "1" if use_legend else "0"
        if parametrise:
            for parameter, value in six.iteritems(parametrise):
                try:
                    next(filter(lambda p: p.name == parameter, self.parameters))
                except Exception:
                    raise Exception("Parameter \"{}\" not found for graph \"{}\"!".format(parameter, self.id))
                details[parameter] = value

        sorted_details = sorted(six.iteritems(details), key=lambda val: val[0])
        if urlencoded:
            return "?{0}".format(urlencode(sorted_details))
        return "?{0}".format("&".join(key + "=" + value for key, value in sorted_details))

    def get_dashboard_link(self, use_legend=True, parametrise=None):
        return self.get_link(use_legend=use_legend, parametrise=parametrise, urlencoded=False)

    def get_full_link(self, time_window=""):
        """
        :time_window: str
        example values - "1h", "1d", "1w", "1w1d1h"
        """
        return "{}/{}&b={}".format(SolomonCluster.STABLE, self.get_link(), time_window)
