#!/skynet/python/bin/python


import os
import sys
import logging

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from argparse import ArgumentParser  # noqa

# add path to venv
if 1:
    root = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
    site_dir = os.path.join(root, 'venv', 'venv', 'lib', 'python2.7', 'site-packages')
    sys.path.insert(1, site_dir)

    # one more hack for processing .pth files from virtualenv site-package
    import site

    site.addsitedir(site_dir)

from balancer_config import GENCFG_API_TIMEOUT  # noqa

TRANSPORTS = [
    "curdb",
    "api",
    "cached-api",
]


def parse_cmd():
    parser = ArgumentParser(description="Generate balancer config")
    parser.add_argument("-b", "--balancer", dest="balancer_name", type=str, required=True,
                        help="Obligatory. Name of balancer")
    parser.add_argument("-m", "--balancer-module", dest="balancer_module_name", type=str, default=None,
                        help="Optional. Balancer module name (in case it does not concur with balancer name")
    parser.add_argument("-t", "--transport", type=str, required=True,
                        choices=TRANSPORTS,
                        help="Obligatory. Transport for generation")
    parser.add_argument("-o", "--output-file", dest="output_file", type=str, required=True,
                        help="Obligagory. Output file path/name")
    parser.add_argument(
        "-p", "--param", dest="params", action='append', type=str, required=False, default=[],
        help=(
            "Optional. Extra parameters for balancer config generator. "
            "Can be specified multiple times to add multiple params"
        )
    )
    parser.add_argument(
        "--save-cache",
        type=str,
        default=None,
        help="GenCFG groups cache file name [in/out]",
    )
    parser.add_argument(
        "--print-stats",
        action="store_true",
        default=False,
        help="Print stats",
    )

    parser.add_argument(
        "-v", "--verbose", action="store_true", default=False,
        help="Be verbose and write some helpful logs",
    )

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    if options.balancer_module_name is None:
        options.balancer_module_name = options.balancer_name

    return options


def make_api_transport():
    from src.transports.gencfg_api_transport import GencfgApiTransport
    return GencfgApiTransport(req_timeout=GENCFG_API_TIMEOUT, attempts=10)


def main(options):
    if options.verbose:
        logging.basicConfig(format="%(asctime)s %(name)s %(levelname)s %(message)s")
        logging.getLogger().setLevel(logging.INFO)

    # import needed data
    sys.path.insert(1, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
    imodule = __import__('configs.%s.%s' % (options.balancer_name, options.balancer_module_name), globals(), locals(),
                         ['process'], -1)

    if options.transport == "curdb":
        sys.path.append(os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))))

        import gencfg  # noqa
        from src.transports.curdb_transport import CurdbTransport

        options.transport = CurdbTransport()
    elif options.transport == "api":
        options.transport = make_api_transport()
    elif options.transport == "cached-api":
        from src.transports.redis_caching_transport import RedisCachingTransport
        api_transport = make_api_transport()
        options.transport = RedisCachingTransport(api_transport)
    else:
        raise Exception("Unknown transport <%s>" % options.transport)

    imodule.process(options)


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
