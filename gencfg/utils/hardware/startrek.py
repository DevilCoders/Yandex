#!/usr/bin/env python
# coding=utf-8

import os
import platform
import sys

from startrek_client import *  # pip install -i https://pypi.yandex-team.ru/simple/ startrek_client

# remove warning about unverified connection
import urllib3

urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)


# https://github.yandex-team.ru/devtools/startrek-python-client
# https://wiki.yandex-team.ru/tracker/api


def init_client():
    if platform.system() == "Windows":
        token_path = "st_token"
    else:
        token_path = "~/.st/token"

    abs_token_path = os.path.expanduser(token_path)
    if not os.path.exists(abs_token_path):
        sys.stderr.write("ST OAuth key is absent: %s\n" % token_path)
        sys.stderr.write("Read more at https://github.yandex-team.ru/devtools/startrek-python-client#configuring\n")
        exit(1)
    key = file(abs_token_path).read().strip()

    return Startrek(token=key, useragent="python")


Client = init_client()


def __filter_none_from_dict(data):
    return dict([(key, val) for key, val in data.iteritems() if val is not None])


def __issue_params(data):
    data = __filter_none_from_dict(data)

    if "type" in data:
        data["type"] = {'name': data["type"]}

    return data


def search_issues(search_filter):
    return Client.issues.find(filter=search_filter)


def get_issue(issue):
    if isinstance(issue, (str, unicode)):
        return Client.issues[issue]

    return issue


def create_issue(
        queue,
        summary,
        type=None,
        description=None,
        assignee=None,
        followers=None,
        components=None,
        project=None,
        tags=None,
):
    issue_dict = __issue_params(locals())

    return Client.issues.create(**issue_dict)


def get_issue_key(issue):
    if isinstance(issue, (str, unicode)):
        return issue

    return issue.key


def link_issues(first, next, relationship):
    get_issue(first).links.create(issue=get_issue_key(next), relationship=relationship)


def unlink_issues(first, next):
    links = get_issue(first).links
    next = get_issue_key(next)

    for link in links:
        if link.object.key == next:
            link.delete()
            return
