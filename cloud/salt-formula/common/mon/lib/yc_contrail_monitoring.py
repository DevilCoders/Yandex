#!/usr/bin/env python3

from collections import namedtuple
from urllib.error import URLError
from urllib.request import urlopen
from defusedxml import ElementTree

from yc_monitoring import report_status_and_exit, Status


class IntrospectionPorts:
    API = 8084
    Control = 8083
    Discovery = 5997
    Schema = 8087
    VRouterAgent = 8085


def load_str(port: str, path: str, host: str="127.0.0.1", debug_file: str=None):
    if debug_file:
        with open(debug_file) as f:
            return f.read()

    url = "http://{}:{}/{}".format(host, port, path)
    try:
        return urlopen(url).read()
    except URLError as e:
        report_status_and_exit(Status.CRIT, "contrail introspection request failed: {!r}".format(e))


def parse_xml(raw_str: str):
    try:
        return ElementTree.fromstring(raw_str)
    except ElementTree.ParseError as e:
        report_status_and_exit(Status.CRIT, "parsing contrail introspection response failed: {!r}".format(e))


def parse_object(element, nt: namedtuple):
    values = [element.find(field).text for field in nt._fields]
    return nt(*values)


def parse_object_list(xml, ctor: callable, xpath: str):
    return [parse_object(e, ctor) for e in xml.findall(xpath)]
