# -*- coding: utf-8 -*-
import jsonobject

from library.python.monitoring.solo.objects.solomon.v2.base import SolomonObject


class MenuItem(SolomonObject):
    title = jsonobject.StringProperty(name="title", required=True, default="")
    url = jsonobject.StringProperty(name="url", required=False, default="")
    selectors = jsonobject.StringProperty(name="selectors", required=True, default="")
    children = jsonobject.ListProperty(SolomonObject, required=False, default=[])


class Menu(SolomonObject):
    __OBJECT_TYPE__ = "Menu"

    id = jsonobject.StringProperty(default="")  # поле id это всегда project_id
    items = jsonobject.ListProperty(MenuItem)

    @property
    def project_id(self):
        return self.id
