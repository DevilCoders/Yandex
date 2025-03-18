# -*- coding: utf-8 -*-
import jsonobject

from library.python.monitoring.solo.objects.solomon.v2.base import SolomonObject


class AggrRules(SolomonObject):
    cond = jsonobject.SetProperty(str)
    target = jsonobject.SetProperty(str)
    function = jsonobject.StringProperty(choices=['SUM', 'LAST'], exclude_if_none=True)


class SensorConf(SolomonObject):
    aggr_rules = jsonobject.SetProperty(AggrRules, name="aggrRules")
    priority_rules = jsonobject.SetProperty(name="priorityRules")
    raw_data_mem_only = jsonobject.BooleanProperty(name="rawDataMemOnly", default=False)


class Service(SolomonObject):
    __OBJECT_TYPE__ = "Service"

    id = jsonobject.StringProperty(name="id", required=True, default="")
    project_id = jsonobject.StringProperty(name="projectId", required=True, default="")
    name = jsonobject.StringProperty(name="name", required=True, default="0")

    type = jsonobject.StringProperty(required=True, default="JSON_GENERIC")
    # ---- PULL only options
    port = jsonobject.IntegerProperty(default=None, exclude_if_none=True)
    path = jsonobject.StringProperty(default=None, exclude_if_none=True)  # TODO: validator path /sensors/4445
    add_ts_args = jsonobject.BooleanProperty(name="addTsArgs", default=None, exclude_if_none=True)
    # ----
    interval = jsonobject.IntegerProperty(default=15)
    grid = jsonobject.IntegerProperty(name="gridSec", default=0)

    sensor_conf = jsonobject.ObjectProperty(SensorConf, name="sensorConf", required=True)
    version = jsonobject.IntegerProperty(name="version", required=True, default=0)
    sensors_ttl_days = jsonobject.IntegerProperty(name="sensorsTtlDays", default=None, exclude_if_none=True)
    sensor_name_label = jsonobject.StringProperty(name="sensorNameLabel", default=None, exclude_if_none=True)
