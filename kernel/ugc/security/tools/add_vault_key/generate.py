#!/usr/bin/env python
# -*- coding: utf-8 -*-
import base64
import datetime

KEY_SIZE = 16
KEY_NAME_PREFIX = 'UGCUPDATE_KEY_'


def get_random(count):
    with open("/dev/random", 'rb') as f:
        return f.read(count)


def get_key():
    return base64.b64encode(get_random(KEY_SIZE)).decode("utf-8")


def get_variable_name(delay_days):
    now = datetime.datetime.now() + datetime.timedelta(days=delay_days)
    current_date = now.strftime("%Y-%m-%d")
    return '{}{}'.format(KEY_NAME_PREFIX, current_date)
