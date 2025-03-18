#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import json
from collections import namedtuple
import urllib2

import gencfg
from core.argparse.parser import ArgumentParserExt
from core.settings import SETTINGS
from gaux.aux_utils import retry_urlopen, correct_pfname
from core.exceptions import UtilRuntimeException


def get_parser():
    parser = ArgumentParserExt("Utility to load gencfg groups infromation from nanny")
    parser.add_argument("-t", "--oauth-key", type=str, required=True,
                        help="Obligatory. Oauth token")
    parser.add_argument("-u", "--nanny-url", type=str, default=None,
                        help="Optional. Nanny url")

    return parser


NannyServiceInfo = namedtuple('NannyServiceInfo', ['name', 'ticket_queue', 'gencfg_groups'])


def main(options):
    url = options.nanny_url if options.nanny_url else SETTINGS.services.nanny.rest.url
    nanny_request = urllib2.Request("%s%s" % (url, SETTINGS.services.nanny.rest.path.services))
    nanny_request.add_header("Authorization", "OAuth %s" % options.oauth_key)
    nanny_request.add_header("Accept", "application/json")
    nanny_request.add_header("Content-Type", "application/json")

    services_info = json.loads(retry_urlopen(5, nanny_request, timeout=10))

    result = []
    for service in services_info["result"]:
        instances_type = service["runtime_attrs"]["content"]["instances"]["chosen_type"]
        if instances_type != "EXTENDED_GENCFG_GROUPS":
            continue

        ticket_queue = service["info_attrs"]["content"].get("queue_id", None)
        if ticket_queue is None:
            continue

        gencfg_groups = map(lambda x: x["name"],
                            service["runtime_attrs"]["content"]["instances"]["extended_gencfg_groups"]["groups"])
        result.append(NannyServiceInfo(name=service["_id"], ticket_queue=ticket_queue, gencfg_groups=gencfg_groups))

    return result


def print_result(result, options):
    del options
    for service in result:
        print "Service %s, ticket queue %s: gencfg groups %s" % (
        service.name, service.ticket_queue, ",".join(service.gencfg_groups))


def jsmain(d):
    options = get_parser().parse_json(d)
    return main(options)


if __name__ == '__main__':
    options = get_parser().parse_cmd()
    result = main(options)
    print_result(result, options)
