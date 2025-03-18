# -*- coding: utf8 -*-

import copy
from collections import OrderedDict
from functools import wraps

# import constants as Constants
from lua_globals import LuaGlobal, LuaProdOrTesting, LuaFuncCall
from parsecontext import ParseContext


class OptionValue(object):
    def __init__(self, value, valuetype=None, obligatory=True, name=None):
        self.value = value
        self.valuetype = valuetype
        self.obligatory = obligatory

        if value is not None and value.__class__ != valuetype:
            if (value.__class__ == LuaGlobal and value.valuetype != valuetype) or value.__class__ != LuaGlobal:
                raise Exception("Invalid default value %s (must be type %s) in opt %s" % (
                    self.value, self.valuetype, name
                ))


def get_options_checker_type(option):
    if isinstance(option, LuaProdOrTesting):
        return get_options_checker_type(option.prod_groups)
    elif isinstance(option, LuaGlobal):
        return option.valuetype
    else:
        return option.__class__


class OptionsChecker(object):
    def __init__(self, options):
        self.options = OrderedDict()
        for name, value, valuetype, obligatory in options:
            self.options[name] = OptionValue(value, valuetype, obligatory, name)

    def __call__(self, f):

        @wraps(f)
        def tmp(extra_options):
            result_options = OrderedDict()
            for name, value in self.options.iteritems():
                if extra_options.get(name) is not None:
                    option_type = get_options_checker_type(extra_options[name])
                    if not isinstance(extra_options[name], LuaFuncCall) and value.valuetype != option_type:
                        raise Exception('''Checker for %s: wrong type %s for option \"%s\" (should be %s)
%s%s
==============================================================''' % (f.__name__, option_type, name, value.valuetype, ParseContext(), extra_options))
                    result_options[name] = extra_options[name]
                else:
                    if value.obligatory:
                        raise Exception('''Checker for %s: Obligatory option %s not found
%s%s
==============================================================''' % (f.__name__, name, ParseContext(), extra_options))
                    else:
                        result_options[name] = copy.copy(value.value)

            for name in extra_options:
                if extra_options[name] is not None and name not in self.options:
                    raise Exception('''Checker for %s: Unknown option %s not in [%s]
%s%s
==============================================================''' % (f.__name__, name, ', '.join(self.options), ParseContext(), extra_options))

            return f(result_options)

        return tmp


def apply_options_checker(data, checker):
    def f(x):
        return x

    return checker(f)(data)


HELPERS = []


class Helper(object):
    def __init__(self, name, descr, options, examples=None):
        self.name = name
        self.descr = descr
        self.options = options
        self.examples = examples
        self.optionsChecker = OptionsChecker(map(lambda x: (x[0], x[1], x[2], x[3]), self.options))

        HELPERS.append(self)

    def _type_to_str(self, type_):
        if type_ == str:
            return 'str'
        if type_ == int:
            return 'int'
        if type_ == dict:
            return 'dict'
        if type_ == bool:
            return 'bool'
        if type_ == float:
            return 'float'
        if type_ == list:
            return 'list'
        if type_ == OrderedDict:
            return 'OrderedDict'

        raise Exception("Unknown type %s" % type_)

    def __str__(self):
        result = ''
        result += '=== %s ===\n\n' % self.name
        result += self.descr + '\n\n'

        result += '<# <table style="border: 0;"> #>\n'
        for name, default_value, type_, obligatory, descr in self.options:

            if obligatory:
                obligatory_string = 'обязательный'
            else:
                obligatory_string = 'default=' + str(default_value) if len(str(default_value)) else "''"

            formatter = '<# <tr> <td style="width: 150px;"> #> %%%%(wacko wrapper=text align=right)\
##%s## &nbsp;%%%% <# </td> <td> #> **%s** //(%s)//\n%s<# </td> </tr> #>\n'
            result += formatter % (self._type_to_str(type_), name, obligatory_string, descr)

        result += '<# </table> #>\n'

        if self.examples:
            # result += "==== Примеры использования ====\n"
            for example_name, example_code in self.examples:
                result += "===== %s =====\n" % example_name
                result += """%%%%(python)
%s
%%%%\n""" % example_code

        return result

    def __call__(self, f):
        return self.optionsChecker(f)
