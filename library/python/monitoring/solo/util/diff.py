# -*- coding: utf-8 -*-
import copy
import json
import logging
from difflib import unified_diff

import click
import six

from library.python.monitoring.solo.util.consts import JUGGLER_MARK_PREFIX

logger = logging.getLogger(__name__)


def sort_internal_lists(d):
    if isinstance(d, list):
        new_list = []
        for el in d:
            new_list.append(sort_internal_lists(el))
        return sorted(new_list, key=lambda x: json.dumps(x, sort_keys=True))
    if isinstance(d, dict):
        for k, v in six.iteritems(d):
            d[k] = sort_internal_lists(v)
        return d
    return d


def print_diff(header, calculated_diff, short=False):
    logger.info(click.style(header, fg="yellow"))
    if not short:
        for line in calculated_diff:
            if line.startswith("-"):
                logger.info(click.style(line, fg="red"))
            elif line.startswith("+"):
                logger.info(click.style(line, fg="green"))
            else:
                logger.info(line)


def get_jsonobject_diff(new_object, old_object):
    if new_object:
        if not isinstance(new_object, dict):
            new_object = new_object.to_json()
    if old_object:
        if not isinstance(old_object, dict):
            old_object = old_object.to_json()
    return calculate_diff(new_object, old_object)


def calculate_diff(new_object, old_object):
    if new_object:
        new_object = sort_internal_lists(new_object)
        new_object_lines = json.dumps(new_object, indent=4, sort_keys=True, ensure_ascii=False).splitlines()
    else:
        new_object_lines = []

    if old_object is not None:
        if new_object:
            old_object = {key: value for key, value in six.iteritems(old_object) if key in new_object}
        old_object = sort_internal_lists(old_object)
        old_object_lines = json.dumps(old_object, indent=4, sort_keys=True, ensure_ascii=False).splitlines()
    else:
        old_object_lines = []

    calculated_diff = list(unified_diff(old_object_lines, new_object_lines,
                                        fromfile="before", tofile="after", lineterm=""))
    return calculated_diff


def juggler_mark_to_tag(mark):
    return '{0}{1}'.format(JUGGLER_MARK_PREFIX, mark)


def get_juggler_diff(controller_juggler_mark, new_check, old_check):
    if old_check:
        old_check = copy.deepcopy(old_check.to_dict())
    else:
        old_check = None

    if new_check:
        new_check = copy.deepcopy(new_check.to_dict())
        del new_check['description']

        # visualizing mark change
        # TODO(lazuka23) prettify
        if not new_check['mark']:
            new_check['mark'] = controller_juggler_mark
        if old_check:
            for i, tag in enumerate(old_check['tags']):
                if tag.startswith(JUGGLER_MARK_PREFIX):
                    new_check['tags'].insert(i, juggler_mark_to_tag(new_check['mark']))
                    break
            else:
                new_check['tags'].append(juggler_mark_to_tag(new_check['mark']))
        del new_check['mark']
    else:
        new_check = None

    return calculate_diff(new_check, old_check)


def get_yasm_diff(new_object, old_object):
    return get_jsonobject_diff(new_object, old_object)
