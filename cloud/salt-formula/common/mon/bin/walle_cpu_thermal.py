#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
based on arcadia wall-e CPU check
https://a.yandex-team.ru/arc/trunk/arcadia/infra/wall-e/checks/walle_cpu_thermal.py

This module resides on linux hwmon infrastructure.
You need kernel with coretemp or k10temp driver for it to work.
"""

import json
import re
import os
import errno
import time
from yc_monitoring import Status, report_status_and_exit

TEMP_THRESHOLD = 87000  # 87.0 degree Celsius

MAX_DESCRIPTION_LENGTH = 700

CHECK_STATE_MAX_AGE = 600
CHECK_STATE_PATH = '/tmp/walle_cpu_thermal_v2.json'
'''
file will contain history of cpu temp with the following structure:
{
    history:
        [
        {
            'timestamp': float(time.time())
            'measurements' => [
                {
                    name: coretemp|k10temp
                    device: <device_link>
                    index: M
                    readings: {
                        '<tempN_label>': int(<tempN_input>)
                    }
                },
            ]
        },
        ...
        ]
}
'''


def get_timestamp():
    return int(time.time())


def read_content(file):
    with open(file, "rt") as src:
        return src.read()


def dump_json_to_msg(reason, status):
    return json.dumps(
        {
                "timestamp": get_timestamp(),
                "reason": reason,
                "status": Status.to_string(status)
        }
    )


def dump_json_to_file(path, dct):
    with open(path, 'w') as f:
        json.dump(dct, f)


class JSONable(object):
    def to_dict(self):
        raise NotImplementedError('This method should be overridden')

    @classmethod
    def from_dict(cls, dct):
        raise NotImplementedError('This method should be overridden')


class HWMonMeasurement(JSONable):
    SUPPORTED_DRIVERS = ['coretemp', 'k10temp']
    HWMON_DEVICE_PATH = '/sys/class/hwmon/hwmon{}'
    HWMONS_PATH = '/sys/class/hwmon'
    SENSOR_RE = re.compile(r'temp[0-9]+_(input|label)')
    HWMON_RE = re.compile(r'hwmon(?P<index>[0-9]+)')

    def __init__(self, index, name=None, device=None, readings=None):
        self._read_only = False
        self.index = index
        self.readings = readings if readings is not None else {}
        self.device = device
        self.name = name
        self._max_temp = None
        self.hwmon_path = self.HWMON_DEVICE_PATH.format(self.index)

        if self.name is None:
            self.read_hwmon()
        else:
            self._read_only = True

    def read_hwmon(self):
        assert self._read_only is False, 'cannot read hwmon twice'
        assert os.path.exists(self.hwmon_path), 'hwmon with index {} does not exist'.format(self.index)

        self.read_hwmon_name()
        self.read_hwmon_device()

        for sensor in self.get_sensors():
            self.readings.update(self.read_sensor(sensor))

        self._read_only = True

    def get_sensors(self):
        sensors = set()
        for filename in os.listdir(self.hwmon_path):
            if self.SENSOR_RE.match(filename):
                sensors.add(filename.split('_')[0])
        return sensors

    def read_sensor(self, sensor):
        return {
            self.read_sensor_label(sensor): self.read_sensor_input(sensor)
        }

    def read_sensor_label(self, sensor):
        return read_content(os.path.join(self.hwmon_path, '{}_label'.format(sensor))).strip()

    def read_sensor_input(self, sensor):
        return int(read_content(os.path.join(self.hwmon_path, '{}_input'.format(sensor))).strip())

    def read_hwmon_name(self):
        self.name = self.read_hwmon_driver(self.hwmon_path)
        return self.name

    def read_hwmon_device(self):
        self.device = os.readlink(os.path.join(self.hwmon_path, 'device')).split('/')[-1]
        return self.device

    def max_temp(self):
        assert self._read_only is True, 'cannot get max_temp from mutable HWMonMeasurment'
        if self._max_temp is None:
            self._max_temp = max(self.readings.values())
        return self._max_temp

    def get_readings_above(self, threshold):
        return {
            'device': self.device,
            'readings': {k: v for k, v in self.readings.items() if v > threshold}
        }

    @staticmethod
    def read_hwmon_driver(hwmon_path):
        try:
            return read_content(os.path.join(hwmon_path, 'name')).strip()
        except IOError as error:
            if error.errno == errno.ENOENT:
                return None
            else:
                raise

    @classmethod
    def find_supported_hwmons(cls):
        mons = []
        for d in os.listdir(cls.HWMONS_PATH):
            match = cls.HWMON_RE.match(d)
            if match:
                index = int(match.groupdict()['index'])
                if cls.read_hwmon_driver(cls.HWMON_DEVICE_PATH.format(index)) in cls.SUPPORTED_DRIVERS:
                    mons.append(index)
        return mons

    @classmethod
    def read_all_supported_hwmons(cls):
        return [HWMonMeasurement(m) for m in cls.find_supported_hwmons()]

    @staticmethod
    def fmt_core_temp(core, temp):
        return "{}: {}C".format(core, temp / 1000)

    def pretty_string(self, threshold):
        high_temp_readings = self.get_readings_above(threshold)

        readings = ", ".join(self.fmt_core_temp(core, temp) for core, temp in high_temp_readings["readings"].items())
        return "{device}: {readings}".format(
            device=high_temp_readings["device"],
            readings=readings
        )

    def to_dict(self):
        return {
            'name': self.name,
            'device': self.device,
            'index': self.index,
            'readings': self.readings
        }

    @classmethod
    def from_dict(cls, dct):
        return cls(
            dct['index'],
            dct['name'],
            dct['device'],
            dct['readings']
        )


class HistoricMeasurement(JSONable):
    def __init__(self, timestamp=None, measurements=None):
        self._max_temp = None
        self.timestamp = timestamp
        self.measurements = measurements
        if self.measurements is None or self.timestamp is None:
            self.read_measurements()

    def read_measurements(self):
        self.measurements = HWMonMeasurement.read_all_supported_hwmons()
        self.timestamp = get_timestamp()

    def max_temp(self):
        if self._max_temp is None:
            self._max_temp = max((m.max_temp() for m in self.measurements))
        return self._max_temp

    def get_measurements_above(self, threshold):
        for m in self.measurements:
            if m.max_temp() > threshold:
                yield m

    def to_dict(self):
        return {
            'timestamp': self.timestamp,
            'measurements': [m.to_dict() for m in self.measurements]
        }

    @classmethod
    def from_dict(cls, dct):
        return cls(
            float(dct['timestamp']),
            [HWMonMeasurement.from_dict(m) for m in dct['measurements']]
        )


class History(JSONable):
    def __init__(self, history=None):
        self.history = history
        self._max_temp = None

    def max_temp(self):
        if self._max_temp is None:
            self._max_temp = max((hm.max_temp() for hm in self.history))
        return self._max_temp

    def last_measurement_above(self, threshold):
        ts = 0
        last_hm = None
        for hm in self.history:
            if hm.max_temp() > threshold:
                if hm.timestamp > ts:
                    ts = hm.timestamp
                    last_hm = hm
        if last_hm is not None:
            return list(last_hm.get_measurements_above(threshold))
        else:
            return None

    def pretty_string(self, threshold):
        measurements = self.last_measurement_above(threshold)

        if measurements:
            measurements_str = ", ".join(m.pretty_string(threshold) for m in measurements)
            if len(measurements_str) > MAX_DESCRIPTION_LENGTH:
                measurements_str = measurements_str[:MAX_DESCRIPTION_LENGTH - 3] + "..."

            return "Critical temperature for {}.".format(measurements_str)
        else:
            return "No high temperature measurements found."

    def to_dict(self):
        return {
            'history': [m.to_dict() for m in self.history]
        }

    @classmethod
    def from_dict(cls, dct):
        """This method will load all actual history ONLY!"""
        now = get_timestamp()
        filtered_history = [
            HistoricMeasurement.from_dict(m)
            for m in dct['history']
            if float(m['timestamp']) > now - CHECK_STATE_MAX_AGE
        ]
        return cls(filtered_history)

    @staticmethod
    def load_history(path):

        try:
            with open(path, 'r') as f:
                history_from_file = json.load(f)
        except OSError:
            history_from_file = None

        if history_from_file is None:
            history_from_file = History([HistoricMeasurement()])
        else:
            history_from_file = History.from_dict(history_from_file)
            history_from_file.history.append(HistoricMeasurement())
        return history_from_file

    @staticmethod
    def save_history(hist, path):
        dump_json_to_file(path, hist.to_dict())


if __name__ == '__main__':
    try:
        history = History.load_history(CHECK_STATE_PATH)
        if history.max_temp() > TEMP_THRESHOLD:
            check_status = Status.CRIT
            description = history.pretty_string(TEMP_THRESHOLD)
        else:
            check_status = Status.OK
            description = 'All temperatures are below threshold.'
        History.save_history(history, CHECK_STATE_PATH)
        report_status_and_exit(check_status, dump_json_to_msg(description, check_status))

    except Exception as e:
        import traceback
        import sys
        _, _, traceback_msg = sys.exc_info()
        traceback_msg = ''.join(traceback.format_exception(type(e), e, traceback_msg))

        traceback_msg_length = len(traceback_msg)

        if traceback_msg_length > MAX_DESCRIPTION_LENGTH:
            half = MAX_DESCRIPTION_LENGTH / 2
            description = traceback_msg[:int(half)] + '\n...\n' + traceback_msg[int(traceback_msg_length)-int(half):]
        else:
            description = traceback_msg

        report_status_and_exit(check_status, dump_json_to_msg(description, check_status))

