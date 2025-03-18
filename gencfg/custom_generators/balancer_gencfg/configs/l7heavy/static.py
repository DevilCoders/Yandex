#!/skynet/python/bin/python
# -*- coding: utf-8 -*-

import src.macroses as Macroses
from src.lua_globals import LuaGlobal
from l7macro import is_term_balancer


def _gen_template_status_check_noc(config_data, regexp=False):
    options = {'regexp': regexp, 'zero_weight_at_shutdown': True}

    if is_term_balancer(config_data):
        options['push_checker_address'] = True

    return [(Macroses.MacroCheckReply, options)]


def template_status_check_noc(config_data):
    return _gen_template_status_check_noc(config_data)


def template_status_check_noc_regexp(config_data):
    return _gen_template_status_check_noc(config_data, regexp=True)
