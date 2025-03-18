# -*- coding: utf-8 -*-
import copy
import time

import six


class Kind(object):
    DGAUGE = "DGAUGE"
    IGAUGE = "IGAUGE"
    COUNTER = "COUNTER"
    HIST = "HIST"
    HIST_RATE = "HIST_RATE"
    RATE = "RATE"


class ValueType:
    SIMPLE = 1
    EXTENDED = 2


class Sensor(object):

    def __init__(self, project, cluster, service, kind=Kind.IGAUGE, value=None, **labels):
        """
        :param project: str
        :param cluster: str
        :param service: str
        :param labels: Dict[str]
        """
        self.labels = labels
        self.labels.update(project=project,
                           cluster=cluster,
                           service=service)

        # for creating sensors
        self.kind = kind
        self.value = value

    @property
    def name(self):
        if "name" in self.labels:
            return self.labels["name"]
        elif "sensor" in self.labels:
            return self.labels["sensor"]
        elif "path" in self.labels:
            return self.labels["path"]

    @staticmethod
    def __value_type(value):
        if isinstance(value, six.string_types):
            return ValueType.SIMPLE
        elif len(value) == 2:
            return ValueType.EXTENDED

        raise Exception('Unsupported value type')

    @staticmethod
    def __as_op_value(value):
        return ('=', value) if Sensor.__value_type(value) == ValueType.SIMPLE else value

    @staticmethod
    def __as_graph_param(value):
        return value if Sensor.__value_type(value) == ValueType.SIMPLE else '{}{}'.format(value[0].replace('=', ''), value[1])

    def __str__(self):
        """
        New style format
        :return: str
        """
        sequence = ['"{0}"{1}"{2}"'.format(k, *self.__as_op_value(v)) for k, v in sorted(six.iteritems(self.labels))]
        sequence = ", ".join(sequence)
        return "".join(["{", sequence, "}"])

    @property
    def selectors(self):
        return "{{{}}}".format(", ".join(["{0}{1}'{2}'".format(k, *self.__as_op_value(v)) for k, v in sorted(six.iteritems(self.labels))]))

    def mutate(self, **kwargs):  # TODO: beautify this
        mutated_labels = {k: v for k, v in six.iteritems(self.labels)}
        mutated_labels.update(kwargs)
        return self.__class__(kind=self.kind, value=self.value, **mutated_labels)

    def serialize(self, pull=True):
        labels_to_serialize = copy.deepcopy(self.labels)
        if pull:
            for label in ["service", "cluster", "project", "host"]:
                labels_to_serialize.pop(label, None)
        else:
            for label in ["service", "cluster", "project"]:
                labels_to_serialize.pop(label, None)
        serialized = {
            "labels": labels_to_serialize,
            "kind": self.kind,
            "value": self.value
        }
        if not pull:
            serialized["ts"] = int(time.time())
        return serialized

    def set_value(self, value):
        mutate = self.mutate()
        if mutate.kind in [Kind.IGAUGE]:
            mutate.value = int(value)
        else:
            mutate.value = float(value)
        return mutate

    def build_sensor_link(self, parameters=(), full_link=True):
        labels = self.labels.copy()
        labels["graph"] = "auto"
        labels.update(dict(parameters))
        sorted_labels = sorted(six.iteritems(labels), key=lambda val: val[0])
        url_path = "?{0}".format("&".join(key + "=" + self.__as_graph_param(value) for key, value in sorted_labels))
        if full_link:
            return "https://solomon.yandex-team.ru/" + url_path
        return url_path
