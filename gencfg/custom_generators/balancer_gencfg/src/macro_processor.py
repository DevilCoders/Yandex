#!/skynet/python/bin/python

import inspect
from collections import OrderedDict

import modules as Modules
import parsecontext
import macroses as Macroses


def is_macro(module):
    return isinstance(module, tuple) and (len(module) == 2) and inspect.isclass(module[0]) and issubclass(module[0], Macroses.IMacro)


def recurse_generate(modules):
    if modules.__class__ in (dict, OrderedDict):
        result = modules
        for k, v in modules.iteritems():
            with parsecontext.StackEntry('Section %s' % k):
                result[k] = recurse_generate(v)
        return result
    elif modules.__class__ == list:
        result = []
        for module in modules:
            if is_macro(module):
                with parsecontext.StackEntry('Macro %s' % module[0].__name__):
                    subresult = recurse_generate(module[0].generate(module[1]))
                    result.extend(subresult)
            else:
                result.append(recurse_generate(module))

        return result
    elif modules.__class__ == tuple:
        result = []
        for module in modules:
            result.append(recurse_generate(module))
        return tuple(result)
    else:
        return modules


def process_macroses(modules):
    with parsecontext.NewStage(parsecontext.ParseContext.STAGES.PROCESS_MACROSES):
        modules = recurse_generate(modules)
    return modules


if __name__ == '__main__':
    print process_macroses([
        (Modules.RegexpDispatcher, [
            ('first', [
                (Modules.Regexp, {'match_fsm': {'URI': '/edit'}}),
                (Modules.Hasher, {'mode': 'subnet'}),
                (Modules.ErrorDocument, {'status': 505}),
            ]),
            ('second', [
                (Macroses.TestMacro, {'mode': 'uuuu', 'uri': 'uuuuuuuu'}),
            ]),
        ]),
    ])
