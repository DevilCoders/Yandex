# -*- coding: utf-8 -*-

import json

from core.hard.loghandlers import SteamLogger


def jsonify(json_str, type_for_log):
    try:
        json_obj = json.loads(json_str, encoding='utf-8')
    except (TypeError, ValueError) as exc:
        json_obj = {}
        SteamLogger.error(
            'Json fails on load:"%(json_str)s" error:"%(err)s"',
            json_str=json_str, err=exc,
            type=type_for_log
        )
    return json_obj
