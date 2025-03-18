# -*- coding: utf-8 -*-
from antirobot.scripts.antirobot_eventlog import event
import re
from library.python import resource

first_cap_re = re.compile('(.)([A-Z][a-z]+)')
all_cap_re = re.compile('([a-z0-9])([A-Z])')


def camel_to_under(name):
    s1 = first_cap_re.sub(r'\1_\2', name)
    return all_cap_re.sub(r'\1_\2', s1).lower()


def create_func(style_, type_):
    def func_bool(x):
        return None if (str(x) not in ("True", "False")) else str(x) == "True"

    def func_float(x):
        return float(x) if x else 0.

    def func_int(x):
        return int(x) if x else 0

    if type_.startswith("T"):
        if style_ == "repeated":  # no idea how to work with repeated nested types. no need though.
            return None
        return lambda x: open_event(x, type_)
    if type_ == "bool":
        return func_bool
    if type_ in ("uint32", "uint64"):
        return func_int
    if type_ == "float" and style_ == "repeated":
        return lambda x: ','.join((str(i) for i in x))
    if type_ == "float":
        return func_float
    return str


def create_ev_scheme(strings):
    """
    Берет на вход содержимое antirobot/idl/antirobot.ev, возвращает
    dict {eventtype: [(function_to_apply_1, fieldname_1), (function_to_apply_2, fieldname_2),..],.. }
    """
    result = {}
    regex_outer = r"message (?P<eventtype>\w+) \{(?P<content>[^}]*)\}"
    regex_inner = "(?P<style>required|optional|repeated) (?P<type>\w+) (?P<name>\w+) "
    matches_outer = re.finditer(regex_outer, strings, re.DOTALL)
    for i in matches_outer:
        result[i.group("eventtype")] = []
        matches_inner = re.finditer(regex_inner, i.group('content'), re.DOTALL)
        for j in matches_inner:
            result[i.group("eventtype")].append((create_func(j.group('style'), j.group('type')), j.group('name')))
    return result


def open_event(ev, eventtype):
    return {camel_to_under(field): func(getattr(ev, field)) for (func, field) in ev_scheme.get(eventtype, []) if func}


def parse_event(event_field):
    ev = event.Event(event_field, None)
    result = dict()
    result["timestamp"] = str(ev.Timestamp)
    result["event_type"] = ev.EventType
    result.update(open_event(ev.Event, ev.EventType))

    header = result.pop("header", {})
    header["timestamp"] = result.pop("timestamp")
    header["event_type"] = result.pop("event_type")
    header["token"] = result.pop("token", "")
    header["rest"] = result
    return header


ev_scheme = create_ev_scheme(resource.find("/antirobot_ev"))
