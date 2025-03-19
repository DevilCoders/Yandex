#!/usr/bin/env python
# -*- coding: utf-8 -*-
""" Make PR body from changelog, stolen from afisha-backend CI """
from __future__ import print_function, unicode_literals, absolute_import
import argparse
import re
from itertools import chain
import requests


PULSE_URL = 'https://github.yandex-team.ru/yandex-icecream/backend/pulse'
PULLS_URL = 'https://api.github.yandex-team.ru/repos/yandex-icecream/backend/pulls' \
            '?state=closed&sort=updated&direction=desc'
PULL_URL_TPL = 'https://github.yandex-team.ru/yandex-icecream/backend/pull/{}'
STARTRECK_TIKET_RE = re.compile(r'[A-Z]{2,}-\d+')
STARTREK_URL = 'https://st.yandex-team.ru'
RELEASE_TITLE_PREFIX = 'Release of'

PR_TEMPLATES = {
    'github': '* {pr.md_pr} {pr.md_title}',
    'conductor': '* {pr.title} ({pr.url}) {pr.st_urls}',
}


def md_link(url, text):
    """ Make link in md format """
    return "[{text}]({url})".format(text=text, url=url)


class PR(object):
    """ Github PR model """
    def __init__(self, number, title, body):
        self.number = number
        self.title = title
        self.body = body

    @property
    def url(self):
        """ Compile PR url """
        return PULL_URL_TPL.format(self.number)

    @property
    def md_pr(self):
        """ Compile nice PR url in md format """
        return md_link(self.url, '#{}'.format(self.number))

    @property
    def st_tickets(self):
        """ Find ST tickets """
        return sorted(set(chain(STARTRECK_TIKET_RE.findall(self.title),
                                STARTRECK_TIKET_RE.findall(self.title))))

    @property
    def st_urls(self):
        """ Compile ST tickets urls """
        return ','.join("{base}/{ticket}".format(
            base=STARTREK_URL,
            ticket=ticket
        ) for ticket in self.st_tickets)

    @property
    def md_title(self):
        """ Compile nice title in md format """
        # make links to ST tickets
        title = self.title
        for ticket in self.st_tickets:
            url = "{base}/{ticket}".format(base=STARTREK_URL, ticket=ticket)
            link = md_link(url, ticket)
            title = title.replace(ticket, link)
        return title


def get_pulls(release_title_prefix):
    """ Get PRs """
    prs = []
    url = PULLS_URL
    while url:
        req = requests.get(url)
        url = None
        for pullr in req.json():
            if pullr['title'].startswith(release_title_prefix):
                break
            if pullr['merged_at'] is not None:
                prs.append(PR(pullr['number'], pullr['title'], pullr['body']))
        else:
            if 'next' in req.links:
                url = req.links['next']['url']
    return prs


def main(release_title_prefix, fmt):
    """ Fire """
    message = [
        PR_TEMPLATES[fmt].format(pr=pr) for pr in get_pulls(release_title_prefix)
    ]
    print('\n'.join(message).encode('utf-8'))


if __name__ == '__main__':
    ARGPARSER = argparse.ArgumentParser(
        description="Makes release PR text based on github pulse history")
    ARGPARSER.add_argument("prefix", nargs='?', default=RELEASE_TITLE_PREFIX,
                           help="Last release title prefix  (default '%(default)s')")
    ARGPARSER.add_argument("--format", default='github', choices=sorted(PR_TEMPLATES.keys()),
                           help="Output format (default '%(default)s')")
    ARGS = ARGPARSER.parse_args()
    main(ARGS.prefix, ARGS.format)
