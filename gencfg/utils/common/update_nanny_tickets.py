#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import urllib2
import json

import gencfg
from core.argparse.parser import ArgumentParserExt
from core.settings import SETTINGS
from gaux.aux_utils import retry_urlopen, correct_pfname
from core.exceptions import UtilRuntimeException
import core.argparse.types as argparse_types


def get_parser():
    parser = ArgumentParserExt("Utility to load gencfg groups infromation from nanny")
    parser.add_argument("-t", "--oauth-key", type=str, required=True,
                        help="Obligatory. Oauth token")
    parser.add_argument("-u", "--nanny-url", type=str, default=None,
                        help="Optional. Nanny url")
    parser.add_argument("-s", "--subject", type=str, default=None,
                        help="Optional. Subject of ticket message")
    parser.add_argument("-b", "--body", type=argparse_types.floaded_str, default=None,
                        help="Optional. Message body")
    parser.add_argument("-q", "--tickets-queue", type=str, default=None,
                        help="Optional. Nanny tickets queue")
    parser.add_argument("-j", "--json", type=argparse_types.jsondict, default=None,
                        help="Optional. Json with body/subject/ticket queue")

    return parser


def normalize(options):
    if len({options.subject is None, options.body is None, options.tickets_queue is None}) != 1:
        raise Exception("You must specify all or none of options <--subject --body --tickets-queue>")
    if int(options.subject is None) + int(options.json is None) != 1:
        raise Exception("Options --subject and --json are mutually exclusive")


def add_ticket_to_nanny(options, subject, body, tickets_queue):
    json_body = json.dumps({
        "queue_id": tickets_queue,
        "title": subject,
        "desc": body,
    })

    url = options.nanny_url if options.nanny_url else SETTINGS.services.nanny.rest.url
    nanny_request = urllib2.Request("%s/v1/tickets/" % url, json_body)
    nanny_request.add_header("Authorization", "OAuth %s" % options.oauth_key)
    nanny_request.add_header("Accept", "application/json")
    nanny_request.add_header("Content-Type", "application/json")

    retry_urlopen(5, nanny_request, timeout=10)


def main(options):
    if options.json is None:
        add_ticket_to_nanny(options, options.subject, options.body, options.tickets_queue)
    else:
        for elem in options.json["result"]:
            add_ticket_to_nanny(options, elem["ticket_subject"], elem["ticket_body"], elem["ticket_queue"])


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    main(options)
