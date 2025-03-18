# -*- coding: utf-8 -*-
import jsonobject

from library.python.monitoring.solo.objects.solomon.v2.base import SolomonObject


class Host(SolomonObject):
    url_pattern = jsonobject.StringProperty(name="urlPattern", required=True)
    ranges = jsonobject.StringProperty(name="ranges", default="", required=True)
    dc = jsonobject.StringProperty(name="dc", default="", required=True)
    labels = jsonobject.SetProperty(str, name="labels", default=set())


class ConductorTag(SolomonObject):
    name = jsonobject.StringProperty(name="name", required=True)
    labels = jsonobject.SetProperty(str, name="labels")


class NannyGroup(SolomonObject):
    service = jsonobject.StringProperty(name="service", required=True)
    use_fetched_port = jsonobject.BooleanProperty(name="useFetchedPort", required=True)
    port_shift = jsonobject.IntegerProperty(name="portShift", required=True)
    cfg_group = jsonobject.SetProperty(str, name="cfgGroup", default=set())
    labels = jsonobject.SetProperty(str, name="labels", default=set())
    env = jsonobject.StringProperty(name="env", choices=["ADMIN", "PRODUCTION"], default="PRODUCTION")


class ConductorGroup(SolomonObject):
    group = jsonobject.StringProperty(name="group")
    labels = jsonobject.SetProperty(str, name="labels")


class HostUrl(SolomonObject):
    url = jsonobject.StringProperty(name="url", required=True)
    ignore_ports = jsonobject.BooleanProperty(name="ignorePorts", required=True)
    labels = jsonobject.SetProperty(str, name="labels")


class QloudGroup(SolomonObject):
    component = jsonobject.StringProperty()
    environment = jsonobject.StringProperty()
    application = jsonobject.StringProperty()
    project = jsonobject.StringProperty()
    deployment = jsonobject.StringProperty()
    labels = jsonobject.SetProperty(str, name="labels")


class Network(SolomonObject):
    network = jsonobject.StringProperty()
    port = jsonobject.IntegerProperty()
    labels = jsonobject.SetProperty(str, name="labels")


class YpCluster(SolomonObject):
    endpoint_set_id = jsonobject.StringProperty(name="endpointSetId", default="")
    pod_set_id = jsonobject.StringProperty(name="podSetId", default="")
    tvm_label = jsonobject.StringProperty(name="tvmLabel", default="")
    yp_label = jsonobject.StringProperty(name="ypLabel", default="")
    cluster = jsonobject.StringProperty(name="cluster", required=True)
    labels = jsonobject.SetProperty(str, name="labels")


class InstanceGroup(SolomonObject):
    instance_group_id = jsonobject.StringProperty(name="instanceGroupId", required=True)
    folder_id = jsonobject.StringProperty(name="folderId", required=True)
    labels = jsonobject.SetProperty(str, name="labels")


class Cluster(SolomonObject):
    __OBJECT_TYPE__ = "Cluster"

    id = jsonobject.StringProperty(name="id", required=True)
    name = jsonobject.StringProperty(name="name", required=True)
    project_id = jsonobject.StringProperty(name="projectId", required=True)
    port = jsonobject.IntegerProperty(name="port", required=False)
    sensors_ttl_days = jsonobject.IntegerProperty(name="sensorsTtlDays", required=False)
    use_fqdn = jsonobject.BooleanProperty(name="useFqdn", required=True, default=False)
    version = jsonobject.IntegerProperty(name="version", required=True, default=0)
    hosts = jsonobject.SetProperty(Host, name="hosts", default=None, exclude_if_none=True)
    conductor_tags = jsonobject.SetProperty(ConductorTag, name="conductorTags", default=set(), exclude_if_none=True)
    nanny_groups = jsonobject.SetProperty(NannyGroup, name="nannyGroups", default=set(), exclude_if_none=True)
    conductor_groups = jsonobject.SetProperty(ConductorGroup, name="conductorGroups", default=set(),
                                              exclude_if_none=True)
    host_urls = jsonobject.SetProperty(HostUrl, name="hostUrls", default=None, exclude_if_none=True)
    qloud_groups = jsonobject.SetProperty(QloudGroup, name="qloudGroups", default=None, exclude_if_none=True)
    networks = jsonobject.SetProperty(Network, name="networks", default=None, exclude_if_none=True)
    yp_clusters = jsonobject.SetProperty(YpCluster, name="ypClusters", default=None, exclude_if_none=True)
    instance_groups = jsonobject.SetProperty(InstanceGroup, name="instanceGroups", default=None, exclude_if_none=True)
