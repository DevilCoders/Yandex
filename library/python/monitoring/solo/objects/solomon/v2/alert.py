# -*- coding: utf-8 -*-
import jsonobject
from itertools import chain

from library.python.monitoring.solo.objects.solomon.v2.base import SolomonObject
from library.python.monitoring.solo.util.text import drop_spaces

ALERT_LINK_TEMPLATE = "https://solomon.yandex-team.ru/admin/projects/{0}/alerts/{1}"

MDB_PROVIDERS = [
    "managed-postgresql-io-usage",
    "managed-postgresql-oldest-transaction",
    "managed-postgresql-cpu-usage",
    "managed-postgresql-disk-free-bytes-percent",
    "managed-postgresql-master-alive",
    "managed-postgresql-net-usage",
    "managed-postgresql-memory-usage",
]


class Expression(SolomonObject):
    program = jsonobject.StringProperty(default="")
    check_expression = jsonobject.StringProperty(name="checkExpression", default="")

    def __init__(self, *args, **kwargs):
        super(Expression, self).__init__(*args, **kwargs)
        self.program = drop_spaces(self.program)


class PredicateRule(SolomonObject):
    comparison = jsonobject.StringProperty()
    target_status = jsonobject.StringProperty(name="targetStatus")
    threshold_type = jsonobject.StringProperty(name="thresholdType")
    threshold = jsonobject.FloatProperty(default=0.0)


class Threshold(SolomonObject):
    selectors = jsonobject.StringProperty()
    time_aggregation = jsonobject.StringProperty(name="timeAggregation")
    predicate = jsonobject.StringProperty()
    threshold = jsonobject.FloatProperty(default=0)
    predicate_rules = jsonobject.ListProperty(
        PredicateRule,
        name="predicateRules",
        exclude_if_none=True,
    )

    def __init__(self, *args, **kwargs):
        super(Threshold, self).__init__(*args, **kwargs)
        if not self.predicate_rules:
            self.predicate_rules = [
                PredicateRule(
                    comparison=self.predicate,
                    threshold_type=self.time_aggregation,
                    threshold=self.threshold,
                    target_status="ALARM",
                ),
            ]
        else:
            # these parameters should be the same as top (first) predicate rule
            assert not self.predicate or self.predicate == self.predicate_rules[0].comparison
            assert not self.time_aggregation or self.time_aggregation == self.predicate_rules[0].threshold_type
            assert not self.threshold or self.threshold == self.predicate_rules[0].threshold

            self.predicate = self.predicate_rules[0].comparison
            self.time_aggregation = self.predicate_rules[0].threshold_type
            self.threshold = self.predicate_rules[0].threshold


class TextValueParameter(SolomonObject):
    name = jsonobject.StringProperty()
    value = jsonobject.StringProperty()


class Template(SolomonObject):
    template_id = jsonobject.StringProperty(name="templateId", choices=list(chain(MDB_PROVIDERS)))
    template_version_tag = jsonobject.StringProperty(name="templateVersionTag")
    service_provider = jsonobject.StringProperty(name="serviceProvider", default="")
    text_value_parameters = jsonobject.ListProperty(TextValueParameter, name="textValueParameters")


class ServiceProviderAnnotation(SolomonObject):
    cluster = jsonobject.StringProperty(default=None, exclude_if_none=True)
    usage_percent = jsonobject.StringProperty(default=None, exclude_if_none=True)
    free_percent = jsonobject.StringProperty(default=None, exclude_if_none=True)


class Type(SolomonObject):
    expression = jsonobject.ObjectProperty(Expression, default=None, exclude_if_none=True)
    threshold = jsonobject.ObjectProperty(Threshold, default=None, exclude_if_none=True)
    fromTemplate = jsonobject.ObjectProperty(Template, name="fromTemplate", default=None, exclude_if_none=True)

    @classmethod
    def validator(cls, value):
        if all(value[t] is not None for t in cls.properties()):
            raise Exception("Alert type property can't provide both expression and threshold properties!")
        if all(value[t] is None for t in cls.properties()):
            raise Exception("Alert type property must have expression or threshold property!")


class Alert(SolomonObject):
    __OBJECT_TYPE__ = "Alert"

    id = jsonobject.StringProperty(name="id", required=True, default="")
    project_id = jsonobject.StringProperty(name="projectId", required=True, default="")
    name = jsonobject.StringProperty(name="name", required=True, default="0")
    version = jsonobject.IntegerProperty(name="version", required=True, default=0)
    notification_channels = jsonobject.SetProperty(str, name="notificationChannels")
    window_secs = jsonobject.IntegerProperty(name="windowSecs", required=True, default=0)
    delay_seconds = jsonobject.IntegerProperty(name="delaySeconds", required=True, default=0)
    description = jsonobject.StringProperty(name="description", required=True, default="")
    annotations = jsonobject.DictProperty()
    type = jsonobject.ObjectProperty(Type, required=True, validators=Type.validator)

    no_points_policy = jsonobject.StringProperty(
        name="noPointsPolicy",
        choices=["NO_POINTS_DEFAULT",
                 "NO_POINTS_OK",
                 "NO_POINTS_WARN",
                 "NO_POINTS_ALARM",
                 "NO_POINTS_NO_DATA",
                 "NO_POINTS_MANUAL"],
        default="NO_POINTS_DEFAULT",
    )
    resolved_empty_policy = jsonobject.StringProperty(
        name="resolvedEmptyPolicy",
        choices=["RESOLVED_EMPTY_DEFAULT",
                 "RESOLVED_EMPTY_OK",
                 "RESOLVED_EMPTY_WARN",
                 "RESOLVED_EMPTY_ALARM",
                 "RESOLVED_EMPTY_NO_DATA",
                 "RESOLVED_EMPTY_MANUAL"],
        default="RESOLVED_EMPTY_DEFAULT",
    )

    # getters
    def get_link(self):
        return ALERT_LINK_TEMPLATE.format(self.project_id, self.id)

    # noinspection PyUnresolvedReferences
    def on_serialize(self):
        super(Alert, self).on_serialize()
        self.description = drop_spaces(self.description)
        if "description" not in self.annotations:
            self.annotations["description"] = self.description


# TODO(lazuka23): use class Alert
class MultiAlert(Alert):
    __OBJECT_TYPE__ = "MultiAlert"

    group_by_labels = jsonobject.SetProperty(str, name="groupByLabels")
