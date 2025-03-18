# coding: utf-8

import json
import pylua
import sys
import logging

from argparse import ArgumentParser

import balancer_config as bc
from src import utils as su
from src.transports.gencfg_api_transport import GencfgApiTransport


def parse_cmd():
    parser = ArgumentParser(description="Generate balancer config")
    parser.add_argument(
        "-b", "--balancer",
        dest="balancer_name",
        type=str,
        required=True,
        help="Balancer name",
    )
    parser.add_argument(
        "-m", "--balancer-module",
        dest="balancer_module_name",
        type=str,
        default=None,
        help="Balancer module name (in case it does not concur with balancer name)",
    )
    parser.add_argument(
        "-t", "--transport",
        type=str,
        required=True,
        choices=["api"],
        help="Transport for generation",
    )
    parser.add_argument(
        "-o", "--output-file",
        dest="output_file",
        type=str,
        required=True,
        help="Output file path/name",
    )
    parser.add_argument(
        "-p", "--param",
        dest="params",
        action='append',
        type=str,
        required=False,
        default=[],
        help=(
            "Extra parameters for balancer config generator. "
            "Can be specified multiple times to add multiple params"
        )
    )
    parser.add_argument(
        "--backends-json",
        type=str,
        default=None,
        help="backends.json file path",
    )
    parser.add_argument(
        "-v", "--verbose",
        action="count",
        help="Be verbose. Multiple options increases verbosity",
    )
    parser.add_argument(
        "--enable-cache",
        action="store_true",
        default=False,
        help="Enable file cache",
    )
    parser.add_argument(
        "--save-cache",
        type=str,
        default=None,
        help="GenCFG groups cache file name [in/out]",
    )
    parser.add_argument(
        "--use-only-cache",
        action="store_true",
        default=False,
        help="Raises exception if not in GenCFG cache",
    )
    parser.add_argument(
        "--do-not-save-cache",
        action="store_true",
        default=False,
        help="Do not save cache",
    )
    parser.add_argument(
        "--groups-cache",
        type=str,
        default=None,
        help="GenCFG groups access cache file name [in/out]"
    )
    parser.add_argument(
        "--disable-trunk-fallback",
        action="store_true",
        default=False,
        help="Disable trunk fallback",
    )
    parser.add_argument(
        "--print-stats",
        action="store_true",
        default=False,
        help="Print stats",
    )

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    if options.balancer_module_name is None:
        options.balancer_module_name = options.balancer_name

    return options


_TEMPLATE = """\
function string.starts(String,Start)
   return string.sub(String,1,string.len(Start))==Start
end

local function extract_backends()
    {data}
    local res = {{}};

    for key, value in pairs(_G) do
        if key and (
            string.starts(key, 'backends_') or string.starts(key, 'ipdispatch_') or string.starts(key, 'instance_')
        ) then
            res[key] = value;
        end;
    end;
    return res;
end

return require('json').encode(extract_backends())
"""


def main(options):
    if options.verbose >= 1:
        # for debugging multiprocessing stuff
        # logging.basicConfig(format="%(asctime)s %(name)s %(process)s %(thread)s %(levelname)s %(message)s")
        logging.basicConfig(format="%(asctime)s %(name)s %(levelname)s %(message)s")
        logging.getLogger().setLevel(logging.INFO)
        if options.verbose >= 2:
            logging.getLogger().setLevel(logging.DEBUG)

    if options.transport != "api":
        raise Exception("Unsupported transport <{}>".format(options.transport))

    options.transport = GencfgApiTransport(
        req_timeout=bc.GENCFG_API_TIMEOUT,
        attempts=10,
        trunk_fallback=not options.disable_trunk_fallback,
    )
    if options.groups_cache:
        options.transport.mapping.load_groups_cache(options.groups_cache)

    if options.use_only_cache:
        su.GENCFG_CACHE.use_only_cache()

    if options.enable_cache:
        su.GENCFG_CACHE.load(options.save_cache)
    else:
        su.GENCFG_CACHE.disable()

    try:
        imodule = __import__(
            'configs.{}.{}'.format(
                options.balancer_name,
                options.balancer_module_name,
            ),
            globals(), locals(), ['process'], -1
        )
    except ImportError:
        # if running from arcadia binary
        imodule = __import__(
            'gencfg.custom_generators.balancer_gencfg.configs.{}.{}'.format(
                options.balancer_name,
                options.balancer_module_name,
            ),
            globals(), locals(), ['process'], -1
        )

    imodule.process(options)
    if options.backends_json:
        with open(options.backends_json, 'w') as f:

            code = _TEMPLATE.format(data=open(options.output_file).read())
            json_data = json.loads(pylua.eval_raw(code))
            json_data['_'] = [{'host': 'localhost', 'cached_ip': '127.0.0.1', 'port': 1, 'weight': 1}]
            json.dump(json_data, f, indent=4)

    if options.save_cache and not options.do_not_save_cache:
        logging.info("Saving cache...")
        su.GENCFG_CACHE.save()
        logging.info("Cache stats: %s", su.GENCFG_CACHE.get_stats())

    if options.print_stats:
        su.GENCFG_CACHE.print_stats()

    if options.groups_cache:
        options.transport.mapping.save_groups_cache(options.groups_cache)


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
