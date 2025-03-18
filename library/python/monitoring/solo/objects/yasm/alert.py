# -*- coding: utf-8 -*-
import jsonobject

import six
from six.moves import urllib

from library.python.monitoring.solo.objects.yasm.base import YasmObject


URLS_TO_FILTER = [{"title": "Алерт в Головане", "type": "yasm_alert"}, {"title": "График алерта", "type": "graph_url"}, {"title": "Снапшот графика алерта", "type": "screenshot_url"}]


class IntervalModify(YasmObject):
    type = jsonobject.StringProperty()
    quant = jsonobject.IntegerProperty()
    interval_end_offset = jsonobject.IntegerProperty()


class ValueModify(YasmObject):
    type = jsonobject.StringProperty(choices=['max', 'aver', 'min', 'none', 'summ'], required=True)
    window = jsonobject.IntegerProperty(required=True)


class Flaps(YasmObject):
    stable = jsonobject.IntegerProperty()
    critical = jsonobject.IntegerProperty()
    boost = jsonobject.IntegerProperty()


class Child(YasmObject):
    host = jsonobject.StringProperty(required=True)
    service = jsonobject.StringProperty(required=True)
    instance = jsonobject.DefaultProperty(exclude_if_none=True)
    group_type = jsonobject.StringProperty(default="HOST")


def filter_urls(d):
    to_test = d.copy()
    to_test.pop("url", None)
    for url in URLS_TO_FILTER:
        if to_test == url:
            return False
    return True


class JugglerCheck(YasmObject):
    host = jsonobject.StringProperty()
    service = jsonobject.StringProperty()
    namespace = jsonobject.StringProperty()
    meta = jsonobject.DictProperty(exclude_if_none=True)
    tags = jsonobject.ListProperty(jsonobject.StringProperty)
    flaps = jsonobject.ObjectProperty(Flaps, default=None, exclude_if_none=True)
    children = jsonobject.ListProperty(Child, exclude_if_none=True)
    description = jsonobject.StringProperty(exclude_if_none=True)
    mark = jsonobject.StringProperty()

    active = jsonobject.StringProperty(default=None)
    active_kwargs = jsonobject.DictProperty(default=None)
    check_options = jsonobject.DictProperty(default=None)

    aggregator = jsonobject.StringProperty(default="logic_or")
    aggregator_kwargs = jsonobject.DictProperty(exclude_if_none=True)
    refresh_time = jsonobject.IntegerProperty(default=90)
    ttl = jsonobject.IntegerProperty(default=900)
    pronounce = jsonobject.StringProperty(default="")

    notifications = jsonobject.ListProperty(jsonobject.DictProperty, exclude_if_none=True)

    def to_json(self):
        result = super(JugglerCheck, self).to_json()
        result.pop("description", None)  # ignore description diff
        if result.get("meta", {}).get("urls", None) is not None:  # ignore meta.urls diff
            result["meta"]["urls"] = list(filter(filter_urls, result["meta"]["urls"]))
        for field in ["alert_interval", "methods"]:  # deprecated parameters
            result.pop(field, None)
        return result


def validate_tags(d):
    if "itype" not in d:
        raise Exception("'itype' key missed in YasmAlert.tags parameter")
    for v in d.values():
        if not isinstance(v, list):
            raise Exception("YasmAlert.tags(dict) values must be lists")


class YasmAlert(YasmObject):
    __OBJECT_TYPE__ = "YasmAlert"

    name = jsonobject.StringProperty(required=True)
    signal = jsonobject.StringProperty(required=True)
    tags = jsonobject.DictProperty(default=dict(), validators=[validate_tags])

    crit = jsonobject.ListProperty(default=[None, None])
    warn = jsonobject.ListProperty(default=[None, None])

    warn_perc = jsonobject.FloatProperty(exclude_if_none=True)
    crit_perc = jsonobject.FloatProperty(exclude_if_none=True)

    trend = jsonobject.StringProperty(exclude_if_none=True)
    interval = jsonobject.IntegerProperty(exclude_if_none=True)
    interval_modify = jsonobject.ObjectProperty(IntervalModify, default=None, exclude_if_none=True)

    value_modify = jsonobject.ObjectProperty(ValueModify, default=None, exclude_if_none=True)

    mgroups = jsonobject.ListProperty(jsonobject.StringProperty)
    description = jsonobject.StringProperty(exclude_if_none=True)
    juggler_check = jsonobject.ObjectProperty(JugglerCheck, default=None, exclude_if_none=True)

    disaster = jsonobject.BooleanProperty(default=False)

    @property
    def id(self):
        return self.name

    def __init__(self, *args, **kwargs):
        super(YasmAlert, self).__init__(*args, **kwargs)
        if self.juggler_check:
            self.add_yasm_generated_data()

    def add_yasm_generated_data(self):
        """
        YASM adds extra related data when creating alert's juggler_check object.
        This method is only used to synchronize those diffs.
        """
        self.juggler_check.meta = self.juggler_check.meta or {"urls": self.get_urls(), "yasm_alert_name": self.name}
        self.juggler_check.children = self.juggler_check.children or [Child(host="yasm_alert",
                                                                            service=self.name,
                                                                            group_type="HOST")]
        self.juggler_check.service = self.juggler_check.service or self.name
        for key, values in six.iteritems(self.tags):
            # Yasm always appends some juggler tags automatically
            yasm_generated_tags = ["a_{}_{}".format(key, v) for v in values]
            for tag in yasm_generated_tags:
                if tag not in self.juggler_check.tags:
                    self.juggler_check.tags += [tag]

        # Ignore these juggler tags (generated by yasm)
        self.juggler_check.tags = [tag for tag in self.juggler_check.tags
                                   if not tag.startswith("a_yasm_prefix") and not tag.startswith("a_mark_")]

    def get_chart_url(self, screenshot=False):
        filters = ";".join("{}={}".format(k, ",".join(v)) for k, v in six.iteritems(
            dict(signals=[self.signal], hosts=self.mgroups, **self.tags))
        )
        if screenshot:
            base_url = "https://s.yasm.yandex-team.ru"
        else:
            base_url = "https://yasm.yandex-team.ru"

        return base_url + "/chart/" + urllib.parse.quote(filters, safe='')

    def get_urls(self):
        return [
            {
                "title": "Алерт в Головане",
                "type": "yasm_alert",
                "url": "https://yasm.yandex-team.ru/chart-alert/alerts={};".format(self.name)
            },
            {
                "title": "График алерта",
                "type": "graph_url",
                "url": self.get_chart_url()
            },
            {
                "title": "Снапшот графика алерта",
                "type": "screenshot_url",
                "url": self.get_chart_url(screenshot=True)
            }
        ]


class YasmAlertsTemplate(YasmObject):
    __OBJECT_TYPE__ = "YasmAlertsTemplate"

    key = jsonobject.StringProperty()
    owners = jsonobject.ListProperty()
    content = jsonobject.StringProperty()

    @property
    def id(self):
        return self.key
