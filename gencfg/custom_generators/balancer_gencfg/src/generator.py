#!/skynet/python/bin/python

from collections import OrderedDict

from src.transports import InstanceDbTransportHolder
from src import macroses as Macroses
from src import modules as Modules
from src import lua_processor
from src.macro_processor import process_macroses
from src.module_processor import prepare_modules, process_modules
from src.optimizer import add_shared, add_stats_attrs, join_report_modules,\
    add_backends_as_globals, remove_stats_events
from src.lua_printer import print_lua, print_lua_globals
from src.constraint_checker import check_exp_and_remote_log_order


def optimize(lua_extra, modules_as_dict, multiple_uuid_allowed=False):
    """
    :param multiple_uuid_allowed: may be set to True only if unistat is used instead of report handle
    """
    add_stats_attrs(modules_as_dict)
    # Shared sections shound be created before report uuid converted to refers MINOTAUR-1348
    add_shared(modules_as_dict)
    if multiple_uuid_allowed:
        remove_stats_events(modules_as_dict)
    else:
        join_report_modules(modules_as_dict)  # SEPE-12320
    add_backends_as_globals(modules_as_dict, lua_extra)  # GENCFG-231


def check_constraints(modules_extra, lua_extra, modules_as_dict):
    del modules_extra, lua_extra
    check_exp_and_remote_log_order(modules_as_dict)


def write_config(modules_extra, lua_extra, modules_as_dict, fname, raw_config=False):
    f = open(fname, 'w')
    if raw_config:
        import pickle

        pickle.dump((modules_extra, lua_extra, modules_as_dict), f)
    else:
        f.write('%s\n\n%s\n\n%s' % (
            '\n\n'.join(modules_extra.data), print_lua_globals(lua_extra), print_lua(modules_as_dict)))

    f.close()


def read_config(fname, raw_config=False):
    f = open(fname, 'r')
    if raw_config:
        import pickle

        return pickle.load(f)
    else:
        raise Exception("Read non-raw config not implemented")


def recurse_process_cfg(msg, modules):
    if isinstance(modules, dict):
        if 'locations' in modules:
            print msg, id(modules)  # , modules
        else:
            for k, v in modules.iteritems():
                recurse_process_cfg(msg, v)
    elif isinstance(modules, (list, tuple)):
        for i in range(len(modules)):
            recurse_process_cfg(msg, modules[i])


def generate(
    macroses, fname,
    raw_config=False,
    do_optimizations=True,
    create_testing_config=False,
    instance_db_transport=None,
    user_config_prefix='',
    multiple_uuid_allowed=False,
):
    if instance_db_transport is not None:
        InstanceDbTransportHolder.set_transport(instance_db_transport)

    modules = process_macroses(macroses)
    modules = prepare_modules(modules)

    if create_testing_config:
        modules = lua_processor.recurse_modify(modules, lua_processor.choose_testing)
    else:
        modules = lua_processor.recurse_modify(modules, lua_processor.choose_production)

    modules_extra, modules_as_dict = process_modules(modules)

    modules_extra += user_config_prefix

    lua_extra = OrderedDict()

    """
        Do various optimizations, like add shared module, ...
    """
    if do_optimizations:
        optimize(lua_extra, modules_as_dict, multiple_uuid_allowed)

    """
        Extract info on lua globals in order to add expressions like "if VarName == nil then VarName = 12345; end"
    """
    lua_extra.update(lua_processor.process_lua_globals(modules_as_dict))

    """
        Check if dict keys do not intersect with lua keywords
    """
    lua_processor.recurse_process(modules_as_dict, dict(), lua_processor.check_lua_keywords)

    """
        Check specific order of some modules
    """
    check_constraints(modules_extra, lua_extra, modules_as_dict)

    write_config(modules_extra, lua_extra, modules_as_dict, fname, raw_config)


if __name__ == '__main__':
    # cfg = [
    #            (Modules.RegexpDispatcher, [
    #                ('first', [
    #                    (Modules.Regexp, { 'match_fsm' : {'URI' : '/edit' } }),
    #                    (Modules.Hasher, { 'mode' : 'subnet' }),
    #                    (Modules.ErrorDocument, { 'status' : 505 }),
    #                ]),
    #                ('second', [
    #                    (Macroses.TestMacro, { 'mode' : 'uuuu', 'uri' : 'uuuuuuuu' }),
    #                ]),
    #            ]),
    #        ]

    #    cfg = [
    #            (Modules.IpdispatchDispatcher, [
    #                ('remote', [
    #                    (Modules.Ipdispatch, { 'ip' : '*', 'port' : 17140, 'stats_attr' : 'addrsbalancer' }),
    #                    (Modules.Http, { 'stats_attr' : 'addrsbalancer', }),
    #                    (Modules.Static, { 'file' : 'gagaga.txt' }),
    #                ]),
    #            ]),
    #    ]
    cfg = [
        (Modules.Main, {
            'workers': 5,
            'maxconn': 10000,
            'admin_port': 17140,
        },),
        (Modules.Ipdispatch, [
            (
                'first',
                {
                    'ip': '*',
                    'port': 17140,
                    'stats_attr': 'addrsbalancer'
                },
                [
                    (
                        Modules.Balancer,
                        {
                            'backends': [
                                'MSK_ANTIROBOT_ANTIROBOT',
                                'MSK_ANTIROBOT_ANTIROBOT_PRESTABLE',
                                'SAS_ANTIROBOT_ANTIROBOT',
                                'SAS_ANTIROBOT_ANTIROBOT_PRESTABLE',
                            ],
                            '2tier_balancer': True
                        }
                    )
                ]
            )
        ])
    ]

    content = generate(cfg)

    print content
