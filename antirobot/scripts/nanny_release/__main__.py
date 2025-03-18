#! /usr/bin/env python
# -*- coding: utf-8 -*-

import argparse
import os
import sys
import re
import datetime
import getpass
import json
import logging
from urlparse import urljoin
import urllib2
import ssl

from sandbox.common import rest
from sandbox.common.proxy import OAuth

antirobotReleaseType = 'ANTIROBOT_BUNDLE'

def ParseArgs():
    parser = argparse.ArgumentParser(description='Post Antirobot release to Nanny',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter)
    parser.add_argument('--nanny-host', default='https://nanny.yandex-team.ru')
    parser.add_argument('SandboxTask', help='URL to Sandbox task that built Antirobot release')
    parser.add_argument('--mail-to', default='antirobots@yandex-team.ru',
        help='Comma-separated e-mails to send notifications to when release is done')
    parser.add_argument('ChangelogFile', help='File with changelog. First line is used as release title. '
                                          'Other lines - as release description')
    parser.add_argument('--user', default=getpass.getuser(), help='Releaser name')
    parser.add_argument('--oauth-nanny', help='OAuth token for Nanny API. See https://nanny.yandex-team.ru/ui/#/oauth', required=True)
    parser.add_argument('--oauth-sandbox', help='OAuth token for Sandbox API', required=True)
    parser.add_argument('--verbose', action="store_true", help='Be verbose')
    parser.add_argument('--release-status', choices=['prestable', 'stable'], default='prestable',
        help='Type of release, stable or prestable (second is the default value)')
    parser.add_argument('--ttl-to-inf', choices=['true', 'false'], default='true',
        help='Either change ttl of the releasing resource to inf or not, true or false (true is the default value)')

    return parser.parse_args()

def GetSandboxTaskType(sandboxUrl):
    return 'TEAMCITY_RUNNER'

def GetSandboxTaskId(sandboxUrl):
    sbHostRe = r'https?://sandbox.yandex-team.ru'
    sbLocations = [r'/task/(\d+).*', r'/sandbox/tasks/view\?task_id=(\d+)']

    patterns = [sbHostRe + l for l in sbLocations]
    for p in patterns:
        match = re.match(p, sandboxUrl)
        if match:
            return int(match.group(1))

    raise Exception("Sandbox task URL doesn't match any pattern from " + str(patterns))

def ParseChangelog(filename):
    f = sys.stdin if filename == '-' else open(filename, 'rt')
    lines = f.read().split('\n', 1)
    while (len(lines) < 2):
        lines.append('')
    releaseNamePattern = "^Antirobot s\d+/r\d+$"
    releaseNameRegexp = re.compile(releaseNamePattern)
    if releaseNameRegexp.match(lines[0]):
        return lines[0], lines[1]

    raise Exception("First line in changelog doesn't match a pattern " + releaseNamePattern)

def CreateNannyRelease(args, releaseData):
    apiUrl = args.nanny_host.rstrip('/')

    CA_CERTS = '/etc/ssl/certs/ca-certificates.crt'
    urlopenArgs = {}
    if os.path.isfile(CA_CERTS) and hasattr(ssl, 'create_default_context'):
        context = ssl.create_default_context(cafile=CA_CERTS)
        urlopenArgs = {'context': context}

    headers = {
        'Content-Type': 'application/json',
        'Authorization': 'OAuth {}'.format(args.oauth_nanny),
    }

    request = urllib2.Request('{}/v1/requests/sandbox_releases/'.format(apiUrl), headers=headers)
    answer = urllib2.urlopen(request, json.dumps(releaseData), **urlopenArgs)

    return json.loads(answer.read())

def SetInfiniteTTL(releaseData, oauth):
    sandboxClient = rest.Client(auth=OAuth(oauth))
    taskResources = sandboxClient.resource.read({"limit": 100, "task_id": releaseData['task_id']})

    resId = -1
    for resItem in taskResources['items']:
        if resItem['type'] == antirobotReleaseType:
            resId = resItem['url'].split('/')[-1]
            break

    if resId == -1:
        raise Exception('{0} resource for requeted task_id {1} was not found'.format(antirobotReleaseType, releaseData['task_id']))

    sandboxClient.resource[resId].attribute['ttl'].update(name='ttl', value='inf')

def main():
    args = ParseArgs()

    if args.verbose:
        logging.basicConfig(level=logging.INFO)

    subject, description = ParseChangelog(args.ChangelogFile)
    logging.info("Release subject - %s", subject)
    logging.info("Release description - %s", description)

    releaseResource = {
        'type' : antirobotReleaseType,
    }

    releaseData = {
        'timestamp': datetime.datetime.utcnow().isoformat(),
        'task_type': GetSandboxTaskType(args.SandboxTask),
        'task_id': GetSandboxTaskId(args.SandboxTask),
        'release_author': args.user,
        'release_subject': subject,
        'release_status': args.release_status,
        'release_comments': description,
        'release_resources': [releaseResource],
        'notify_email': {
            'to': args.mail_to.split(','),
            'cc': [],
        },
    }

    # emulation of the create_release() method from sandbox NannyClient
    reqJson = CreateNannyRelease(args, releaseData)

    logging.info("Got request %s", json.dumps(reqJson))

    requestId = reqJson['id']
    print urljoin(args.nanny_host, '/ui/#/r/' + requestId)

    # put inf ttl attribute for released resource
    if args.ttl_to_inf == 'true':
        SetInfiniteTTL(releaseData, args.oauth_sandbox)

if __name__ == "__main__":
    main()
