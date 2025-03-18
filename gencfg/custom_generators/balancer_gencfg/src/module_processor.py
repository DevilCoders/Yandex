#!/skynet/python/bin/python
from collections import OrderedDict

import modules as Modules
from lua_globals import LuaAnonymousKey, LuaGlobal
from parsecontext import ParseContext, NewStage
from utils import load_listen_addrs


# some initial work like filling main module addrs
def prepare_modules(modules):
    main_module, main_module_params = modules[0]
    if main_module != Modules.Main:
        raise Exception("Wrong start module %s" % main_module)

    ipdispatch_module, ipdispatch_module_params = modules[1]
    if ipdispatch_module != Modules.Ipdispatch:
        raise Exception("Wrong ipdispatch module %s" % ipdispatch_module)

    main_module_params['addrs'] = OrderedDict()

    for name, params, submodules in ipdispatch_module_params:
        listen_addrs = load_listen_addrs([params])
        for ip, port in listen_addrs:
            if 'disabled' in params:
                vdict = OrderedDict([('ip', ip), ('port', port), ('disabled', params['disabled']), ])
            else:
                vdict = OrderedDict([('ip', ip), ('port', port), ])

            main_module_params['addrs'][LuaAnonymousKey('--[[ SLB: %s ]]--' % params.get('slb', ''))] = vdict

    main_module_params['addrs'] = LuaGlobal('instance_addrs', main_module_params['addrs'])

    return modules


def process_modules(modules):
    Modules.BALANCER_BACKENDS_BY_HASH.clear()

    with NewStage(ParseContext.STAGES.PROCESS_MODULES):
        result_prefix, result_dict = Modules.SeqProcessor(modules)

    return result_prefix, result_dict


if __name__ == '__main__':
    modules = [
        (Modules.Balancer, {'backends': 'ws3-300'}),
    ]

    # modules = [
    #        (Modules.RegexpDispatcher, [
    #            ('first', [
    #                (Modules.Regexp, { 'match_fsm' : {'URI' : '/edit' } }),
    #                (Modules.Hasher, { 'mode' : 'subnet' }),
    #                (Modules.ErrorDocument, { 'status' : 505 }),
    #            ]),
    #            ('second', [
    #                (Modules.Regexp, { 'match_fsm' : {'URI' : '/clck/.*', } }),
    #                (Modules.ErrorDocument, { 'status' : 404 }),
    #            ]),
    #        ]),
    #    ]

    '''regexp :
    - first :
        match_fsm :
            URI : '/edit'
        hasher :
            mode : subnet
        error_document:
            status : 505
    - second :
        match_fsm :
            URI : '/clck.*'
        error_document :
            status : 404'''

    print process_modules(modules)
