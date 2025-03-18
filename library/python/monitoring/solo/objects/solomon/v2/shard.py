# -*- coding: utf-8 -*-
import jsonobject

from library.python.monitoring.solo.objects.solomon.v2.base import SolomonObject


class Shard(SolomonObject):
    __OBJECT_TYPE__ = "Shard"

    id = jsonobject.StringProperty(name="id", required=True, default="")
    project_id = jsonobject.StringProperty(name="projectId", required=True, default="")
    cluster_id = jsonobject.StringProperty(name="clusterId")
    service_id = jsonobject.StringProperty(name="serviceId")
    sensors_ttl_days = jsonobject.IntegerProperty(name="sensorsTtlDays")
    sensor_name_label = jsonobject.StringProperty(name="sensorNameLabel", default="")
    version = jsonobject.IntegerProperty(name="version", required=True, default=0)
