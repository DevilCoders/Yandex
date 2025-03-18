# -*- coding: utf-8 -*-
import jsonobject

from library.python.monitoring.solo.objects.yasm.base import YasmObject


class Link(YasmObject):
    """
    https://wiki.yandex-team.ru/golovan/userdocs/panels/format/#link
    """
    title = jsonobject.StringProperty(required=True)
    url = jsonobject.StringProperty(required=True)


class AbstractChart(YasmObject):
    """
    https://wiki.yandex-team.ru/golovan/userdocs/panels/format/#abstractchart
    """
    id = jsonobject.StringProperty(required=True)
    type = jsonobject.StringProperty(choices=["graphic", "alert", "check", "iframe" "text"])
    width = jsonobject.IntegerProperty()
    height = jsonobject.IntegerProperty()
    row = jsonobject.IntegerProperty()
    col = jsonobject.IntegerProperty()
    description = jsonobject.StringProperty(default="")
    links = jsonobject.ListProperty(Link)


class YAxis(YasmObject):
    """
    https://wiki.yandex-team.ru/golovan/userdocs/panels/format/#yaxis
    """
    min_value = jsonobject.FloatProperty(name="minValue", exclude_if_none=True)
    max_value = jsonobject.FloatProperty(name="maxValue", exclude_if_none=True)
    log = jsonobject.IntegerProperty(exclude_if_none=True)


class Transform(YasmObject):
    """
    https://wiki.yandex-team.ru/golovan/userdocs/panels/format/#transform
    """
    name = jsonobject.StringProperty(choices=["avg_with_previous", "moving_median", "moving_avg", "moving_max", "negative"])
    radius = jsonobject.IntegerProperty(exclude_if_none=True)


class MappingChart(YasmObject):
    # GraphicChart attributes
    min_value = jsonobject.FloatProperty(name="minValue", exclude_if_none=True)
    max_value = jsonobject.FloatProperty(name="maxValue", exclude_if_none=True)
    title = jsonobject.StringProperty(exclude_if_none=True)
    stacked = jsonobject.BooleanProperty(exclude_if_none=True)
    normalize = jsonobject.BooleanProperty(exclude_if_none=True)
    disable_domain_errors = jsonobject.BooleanProperty(name="disableDomainErrors", exclude_if_none=True)
    y_axis = jsonobject.ObjectProperty(YAxis, name="yAxis", default=None, exclude_if_none=True)

    # TextChart attributes
    text = jsonobject.StringProperty(exclude_if_none=True)
    color = jsonobject.StringProperty(exclude_if_none=True)
    bg_color = jsonobject.StringProperty(name="bgColor", exclude_if_none=True)
    show_signal_name = jsonobject.BooleanProperty(name="showSignalName", exclude_if_none=True)
    show_signal_value = jsonobject.BooleanProperty(name="showSignalValue", exclude_if_none=True)


class MappingSignal(YasmObject):
    title = jsonobject.StringProperty(exclude_if_none=True)
    color = jsonobject.StringProperty(exclude_if_none=True)
    width = jsonobject.FloatProperty(exclude_if_none=True)
    delta = jsonobject.IntegerProperty(exclude_if_none=True)
    y_axis = jsonobject.IntegerProperty(name="yAxis", exclude_if_none=True)
    fraction_size = jsonobject.IntegerProperty(name="fractionSize", exclude_if_none=True)
    normalizable = jsonobject.BooleanProperty(exclude_if_none=True)
    skip_in_scale = jsonobject.BooleanProperty(name="skipInScale", exclude_if_none=True)
    filled = jsonobject.FloatProperty(exclude_if_none=True)
    active = jsonobject.BooleanProperty(exclude_if_none=True)
    transforms = jsonobject.ListProperty(Transform)


class Mapping(YasmObject):
    """
    https://wiki.yandex-team.ru/golovan/userdocs/panels/format/#mapping
    """
    range = jsonobject.ListProperty(required=True)
    chart = jsonobject.ObjectProperty(MappingChart, required=True)
    signal = jsonobject.ObjectProperty(MappingSignal, required=True)


class Signal(YasmObject):
    """
    https://wiki.yandex-team.ru/golovan/userdocs/panels/format/#signal
    """
    tag = jsonobject.StringProperty()
    host = jsonobject.StringProperty()
    name = jsonobject.StringProperty(required=True)
    title = jsonobject.StringProperty(exclude_if_none=True)
    color = jsonobject.StringProperty(exclude_if_none=True)
    width = jsonobject.FloatProperty(exclude_if_none=True)
    delta = jsonobject.IntegerProperty(exclude_if_none=True)
    alert_name = jsonobject.StringProperty(name="alertName", exclude_if_none=True)
    y_axis = jsonobject.IntegerProperty(name="yAxis", exclude_if_none=True)
    fraction_size = jsonobject.IntegerProperty(name="fractionSize", exclude_if_none=True)
    normalizable = jsonobject.BooleanProperty(exclude_if_none=True)
    skip_in_scale = jsonobject.BooleanProperty(name="skipInScale", exclude_if_none=True)
    filled = jsonobject.FloatProperty(exclude_if_none=True)
    active = jsonobject.BooleanProperty(exclude_if_none=True)
    continuous = jsonobject.BooleanProperty(exclude_if_none=True)
    transforms = jsonobject.ListProperty(Transform, exclude_if_none=True)
    mapping = jsonobject.ListProperty(Mapping, exclude_if_none=True)

    def __str__(self):
        return self.name


class Const(YasmObject):
    """
    https://wiki.yandex-team.ru/golovan/userdocs/panels/format/#const
    """
    value = jsonobject.FloatProperty(required=True)
    title = jsonobject.StringProperty(exclude_if_none=True)
    color = jsonobject.StringProperty(exclude_if_none=True)
    width = jsonobject.FloatProperty(exclude_if_none=True)
    y_axis = jsonobject.IntegerProperty(name="yAxis", exclude_if_none=True)
    active = jsonobject.BooleanProperty(exclude_if_none=True)


class GraphicChart(AbstractChart):
    """
    https://wiki.yandex-team.ru/golovan/userdocs/panels/format/#graphicchart
    """
    title = jsonobject.StringProperty(exclude_if_none=True)
    period = jsonobject.IntegerProperty(choices=[5, 300, 3600, 10800, 21600, 43200, 86400],
                                        exclude_if_none=True)
    range = jsonobject.IntegerProperty(exclude_if_none=True)
    min_value = jsonobject.FloatProperty(name="minValue")
    max_value = jsonobject.FloatProperty(name="maxValue")
    stacked = jsonobject.BooleanProperty(exclude_if_none=True)
    normalize = jsonobject.BooleanProperty(exclude_if_none=True)
    disable_domain_errors = jsonobject.BooleanProperty(name="disableDomainErrors", exclude_if_none=True)
    y_axis = jsonobject.ObjectProperty(YAxis, name="yAxis", default=None, exclude_if_none=True)
    signals = jsonobject.ListProperty(Signal)
    consts = jsonobject.ListProperty(Const)

    def __init__(self, *args, **kwargs):
        super(GraphicChart, self).__init__(*args, **kwargs)
        self.type = "graphic"


class AlertChart(AbstractChart):
    """
    https://wiki.yandex-team.ru/golovan/userdocs/panels/format/#alertchart
    """
    title = jsonobject.StringProperty(exclude_if_none=True)
    name = jsonobject.StringProperty(required=True)
    fraction_size = jsonobject.IntegerProperty(name="fractionSize", exclude_if_none=True)
    show_title = jsonobject.BooleanProperty(name="showTitle", default=True)
    show_thresholds = jsonobject.BooleanProperty(name="showThresholds", exclude_if_none=True)

    def __init__(self, *args, **kwargs):
        super(AlertChart, self).__init__(*args, **kwargs)
        self.type = "alert"


class CheckChart(AbstractChart):
    """
    https://wiki.yandex-team.ru/golovan/userdocs/panels/format/#checkchart
    """
    title = jsonobject.StringProperty(exclude_if_none=True)
    host = jsonobject.StringProperty(required=True)
    service = jsonobject.StringProperty(required=True)

    def __init__(self, *args, **kwargs):
        super(CheckChart, self).__init__(*args, **kwargs)
        self.type = "check"


class IframeChart(AbstractChart):
    """
    https://wiki.yandex-team.ru/golovan/userdocs/panels/format/#iframechart
    """
    src = jsonobject.StringProperty(required=True)

    def __init__(self, *args, **kwargs):
        super(IframeChart, self).__init__(*args, **kwargs)
        self.type = "iframe"


class TextChart(AbstractChart):
    """
    https://wiki.yandex-team.ru/golovan/userdocs/panels/format/#textchart
    """
    text = jsonobject.StringProperty(exclude_if_none=True)
    color = jsonobject.StringProperty(exclude_if_none=True)
    bg_color = jsonobject.StringProperty(name="bgColor", exclude_if_none=True)
    signal = jsonobject.ObjectProperty(Signal, default=None, exclude_if_none=True)
    show_signal_name = jsonobject.BooleanProperty(name="showSignalName", exclude_if_none=True)
    show_signal_value = jsonobject.BooleanProperty(name="showSignalValue", exclude_if_none=True)

    def __init__(self, *args, **kwargs):
        super(TextChart, self).__init__(*args, **kwargs)
        self.type = "text"
