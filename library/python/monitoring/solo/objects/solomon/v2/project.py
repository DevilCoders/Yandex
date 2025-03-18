# -*- coding: utf-8 -*-
import jsonobject

from library.python.monitoring.solo.objects.solomon.v2.base import SolomonObject


class ACL(SolomonObject):
    login = jsonobject.StringProperty(default="login")
    permissions = jsonobject.SetProperty(str, default={"READ", "WRITE", "CONFIG_UPDATE", "CONFIG_DELETE"})


class Project(SolomonObject):
    __OBJECT_TYPE__ = "Project"

    id = jsonobject.StringProperty(name="id", required=True, default="")
    name = jsonobject.StringProperty(name="name", required=True, default="0")
    description = jsonobject.StringProperty(name="description", default="")
    owner = jsonobject.StringProperty(name="owner", exclude_if_none=True, default=None)
    abc_service = jsonobject.StringProperty(name="abcService", required=True)
    acl = jsonobject.SetProperty(ACL)
    only_auth_push = jsonobject.BooleanProperty(name="onlyAuthPush", default=False)
    only_sensor_name_shards = jsonobject.BooleanProperty(name="onlySensorNameShards", default=False)
    only_new_format_writes = jsonobject.BooleanProperty(name="onlyNewFormatWrites", default=False)
    only_new_format_reads = jsonobject.BooleanProperty(name="onlyNewFormatReads", default=False)
    version = jsonobject.IntegerProperty(name="version", required=True, default=0)

    @property
    def project_id(self):
        return self.id
