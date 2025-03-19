#!/usr/bin/env python
# -*- coding: utf-8 -*-
from utils.command import Command, CommandResult, ResultCode
from common.functions import get_config_option_getter


class GetConfigOption(Command):
    cmd_name = "get_config_option"

    def __call__(self):
        options = self.config.get_redisctl_options('options')
        options_getter = get_config_option_getter(self.config)
        values = []
        result_code = ResultCode.SUCCESS
        for option in options:
            value = options_getter(option)
            if value is None:
                result_code = ResultCode.ERROR
            if value in ('', None):
                value = "''"
            values.append(value)
        return CommandResult(result_print=",".join(values), result_code=result_code)
