#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import string
import re
from collections import defaultdict

import gencfg
from core.db import CURDB
from gaux.aux_utils import run_command, prompt
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types


def get_parser():
    parser = ArgumentParserExt(description="Send mail to all related admins")
    parser.add_argument("-s", "--subject", dest="subject", type=str, required=True,
                        help="Obligatory. Mail subject")
    parser.add_argument("-c", "--content", dest="content", type=argparse_types.floaded_str, required=True,
                        help="Obligatory. File with content")
    parser.add_argument("-r", "--recipients", dest="recipients", type=str, default='',
                        help="Optional. Recipients list")
    parser.add_argument("-g", "--load-recipients-from-groups", dest="load_recipients_from_groups", action="store_true",
                        default=False,
                        help="Optional. Generate recipients from groups, mentioned in the message")
    parser.add_argument("-o", "--load-recipients-from-hosts", dest="load_recipients_from_hosts", action="store_true",
                        default=False,
                        help="Optional. Generate recipients from hosts, mentioned in the message")
    parser.add_argument("-d", "--per-admin-objects", dest="per_admin_objects", action="store_true", default=False,
                        help="Optional. Add detailed statistics (for every admin list his hosts or groups")
    parser.add_argument("-p", "--prompt", action="store_true", default=False,
                        help="Optional. Prompt before sending mails")
    parser.add_argument("-v", "--verbose", action="count", dest="verbose_level", default=0,
                        help="Optional. Verbose mode. Multiple -v options increase the verbosity. The maximum is 1.")

    return parser


def normalize(options):
    if int(options.recipients != '') + int(options.load_recipients_from_groups) + int(
            options.load_recipients_from_hosts) < 1:
        raise Exception("You must use at least one of <--recipients, --load-recipients-from-groups, --load-recipients-from-hosts> option")

    if len(options.recipients) > 0:
        options.recipients = options.recipients.split(',')
    else:
        options.recipients = []


def main(options):
    GROUP_PATTERN = """[!"#$%&'()*+,./:;<=>?@[\]^`{}~ \n\t]"""
    HOST_PATTERN = """[!"#$%&'()*+,/:;<=>?@[\]^`{}~ \n\t]"""
    MAIL_PATTERN = re.compile(r'^([a-z0-9_-]+\.)*[a-z0-9_-]+$')

    per_admin_objects = defaultdict(list)
    if options.load_recipients_from_groups:
        mentioned_groups = filter(lambda x: CURDB.groups.has_group(x), set(
            map(lambda x: x.strip(string.punctuation).upper(), re.split(GROUP_PATTERN, options.content))))
        for group in mentioned_groups:
            group = CURDB.groups.get_group(group)
            options.recipients.extend(group.card.owners)
            per_admin_objects[','.join(sorted(group.card.owners))].append(group.card.name)
    if options.load_recipients_from_hosts:
        mentioned_hosts = filter(lambda x: CURDB.hosts.has_host(x), map(lambda x: x.strip(string.punctuation).lower(),
                                                                        re.split(HOST_PATTERN, options.content)))
        for host in mentioned_hosts:
            for group in CURDB.groups.get_host_groups(host):
                options.recipients.extend(group.card.owners)
                per_admin_objects[','.join(sorted(group.card.owners))].append(host)
    options.recipients = sorted(list(set(options.recipients)))

    if options.per_admin_objects:
        options.content += """
====== Per admin objects ========
%s
=================================""" % ('\n'.join(
            map(lambda x: "%s: %s" % (x, ', '.join(sorted(list(set(per_admin_objects[x]))))),
                per_admin_objects.keys())))

    if len(options.recipients) == 0:
        raise Exception("Empty recipients list")

    if (options.load_recipients_from_groups or options.load_recipients_from_hosts) and options.prompt:
        question = "Continue with the following recipients: %s?" % (','.join(options.recipients))
        if not prompt(question):
            print "Can not continue, exiting ..."
            sys.exit(1)

    options.recipients = filter(lambda x: MAIL_PATTERN.match(x), options.recipients)

    if options.verbose_level > 0:
        print "Mail recipients: %s" % " ".join(map(lambda x: "%s@yandex-team.ru" % x, options.recipients))
    mailargs = ['mail', '-a', 'Content-Type: text/plain; charset=utf8', '-s', options.subject] + map(
        lambda x: '%s@yandex-team.ru' % x, options.recipients)
    run_command(mailargs, timeout=10, stdin=options.content)

    return {
        'recipients': options.recipients,
    }


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    main(options)
