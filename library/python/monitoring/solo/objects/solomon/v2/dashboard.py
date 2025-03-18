# -*- coding: utf-8 -*-
import jsonobject

try:
    from urllib import urlencode
except ImportError:
    from urllib.parse import urlencode

from library.python.monitoring.solo.objects.solomon.v2.base import SolomonObject
from library.python.monitoring.solo.objects.solomon.v2.graph import Parameter, SolomonCluster


class Panel(SolomonObject):
    type = jsonobject.StringProperty(choices=["IFRAME", "MARKDOWN"], default="IFRAME")
    title = jsonobject.StringProperty(default="")
    subtitle = jsonobject.StringProperty(default="")
    url = jsonobject.StringProperty(default="")
    markdown = jsonobject.StringProperty(default="")
    rowspan = jsonobject.IntegerProperty(default=0)
    colspan = jsonobject.IntegerProperty(default=0)


class Row(SolomonObject):
    panels = jsonobject.ListProperty(Panel)


class Dashboard(SolomonObject):
    __OBJECT_TYPE__ = "Dashboard"

    id = jsonobject.StringProperty(name="id", required=True, default="")
    project_id = jsonobject.StringProperty(name="projectId", required=True, default="")
    name = jsonobject.StringProperty(name="name", required=True, default="0")
    description = jsonobject.StringProperty(name="description", required=True, default="")
    height_multiplier = jsonobject.FloatProperty(name="heightMultiplier", default=1.0)
    parameters = jsonobject.SetProperty(Parameter, default=set())
    rows = jsonobject.ListProperty(Row)
    version = jsonobject.IntegerProperty(name="version", required=True, default=0)

    def get_link(self, urlencoded=True, extra_params=None):
        params = {
            "project": self.project_id,
            "dashboard": self.id
        }
        if extra_params is not None:
            params.update(extra_params)
        if urlencoded:
            url_params = "?{0}".format(urlencode(params))
        else:
            url_params = "?{0}".format("&".join(key + "=" + value for key, value in params))
        return "{}/{}".format(SolomonCluster.STABLE, url_params)
