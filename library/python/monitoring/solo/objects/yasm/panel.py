# -*- coding: utf-8 -*-
import jsonobject

from library.python.monitoring.solo.objects.yasm.base import YasmObject
from library.python.monitoring.solo.objects.yasm.chart import AbstractChart


class YasmPanel(YasmObject):
    """
    https://wiki.yandex-team.ru/golovan/userdocs/panels/format/#panel
    """
    __OBJECT_TYPE__ = "YasmPanel"

    key = jsonobject.StringProperty(required=True)
    user = jsonobject.StringProperty(required=True)
    editors = jsonobject.ListProperty(jsonobject.StringProperty)
    type = jsonobject.StringProperty()
    title = jsonobject.StringProperty(exclude_if_none=True)
    charts = jsonobject.ListProperty(AbstractChart)
    description = jsonobject.StringProperty(default="")
    reload_interval = jsonobject.IntegerProperty(name="reloadInterval", default=30)
    fit_to_viewport = jsonobject.BooleanProperty(name="fitToViewport", default=False)
    legend_sorting_descend = jsonobject.BooleanProperty(name="legendSortingDescend", exclude_if_none=True)
    legend_sorting_type = jsonobject.StringProperty(name="legendSortingType", choices=["value", "alphabet"])

    def __init__(self, *args, **kwargs):
        super(YasmPanel, self).__init__(*args, **kwargs)
        self.type = "panel"

    @property
    def id(self):
        return self.key

    def to_json(self):
        json_data = super(YasmPanel, self).to_json()
        json_data.pop("key")  # key parameter can not be modified
        return json_data


class YasmPanelsTemplate(YasmObject):
    """
    https://wiki.yandex-team.ru/golovan/userdocs/templatium/panels/api/
    """
    __OBJECT_TYPE__ = "YasmPanelsTemplate"

    key = jsonobject.StringProperty()
    owners = jsonobject.ListProperty()
    content = jsonobject.StringProperty()

    @property
    def id(self):
        return self.key
