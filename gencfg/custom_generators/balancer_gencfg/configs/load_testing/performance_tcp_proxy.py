from collections import OrderedDict

import src.modules as Modules
import src.macroses as Macroses
from src.generator import generate
from src.lua_globals import LuaGlobal

PORT = 8080


def process(options):
    cfg = [
        (Modules.Main, {
            'workers': LuaGlobal('workers', 2),
            'maxconn': 20000,
            'port': PORT,
            'admin_port': PORT,
            'enable_reuse_port': True
        }),
        (Modules.Ipdispatch, [
            ('default', {'ip': '*', 'port': LuaGlobal('port_localips', PORT)}, [
                (Macroses.Errorlog, {
                    'port': PORT
                }),
                (Macroses.Accesslog, {
                    'port': PORT
                }),
                (Modules.Report, {
                    'uuid': 'service_total',
                    'stats': 'report',
                }),
                (Modules.Balancer2, {
                    'backends': ['SAS_WEB_RUS_YALITE', 'VLA_WEB_RUS_YALITE'],
                    'balancer_type': 'rr',
                    'connection_attempts': 2,
                    'policies': OrderedDict([('unique_policy', {})]),
                }),
             ]),
        ]),
    ]

    generate(cfg, options.output_file, instance_db_transport=options.transport)
