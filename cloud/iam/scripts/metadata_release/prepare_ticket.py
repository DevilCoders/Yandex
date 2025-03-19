#!/usr/bin/env python3

import os
import re
import subprocess
import webbrowser
from datetime import date
from urllib.parse import urlencode

import requests

ST_API_URL = "https://st-api.yandex-team.ru"
ST_URL = "https://st.yandex-team.ru"
yaml_metadata_files = 'private-api/yandex/cloud/priv/**.yaml'


def find_cloud_go_path():
    explicit = os.environ.get('CLOUD_GO_PATH', None)
    if explicit:
        return explicit
    home = os.environ.get('HOME')
    go_path = os.environ.get('GOPATH', None)
    package = 'bb.yandex-team.ru/cloud/cloud-go'
    candidates = []
    if go_path:
        candidates.append(f'{go_path}/src/{package}')
    if home:
        candidates.append(f'{home}/go/src/{package}')

    for c in candidates:
        if os.path.isdir(c + '/.git'):
            return c

    raise Exception("path to cloud-go git repo could not be guessed and explicit env variable CLOUD_GO_PATH is not set")


def find_st_oauth_token():
    token = os.environ.get('ST_OAUTH_TOKEN', None)
    if token:
        return token
    file = os.environ["HOME"] + "/.config/oauth/st"
    if os.path.isfile(file):
        with open(file, "r") as f:
            return f.readline().strip()
    raise Exception("Either env variable 'ST_OAUTH_TOKEN' or file '~/.config/oauth/st' is required")


def find_previous_ticket():
    req = {
        "filter": {
            "queue": "CLOUD",
            "type": 12,  # Release
            "components": 2369,  # IAM
            "tags": ["iam-metadata"]
        },
        "order": "-created",
    }

    token = find_st_oauth_token()
    headers = {"Authorization": f"OAuth {token}"}
    resp = requests.post(f"{ST_API_URL}/v2/issues/_search", json=req, headers=headers)
    resp.raise_for_status()
    for ticket in resp.json():
        return ticket
    raise Exception("Previous ticket not found in startrek")


def find_previous_ticket_and_commit():
    ticket_from_env = os.environ.get('PREVIOUS_RELEASE_TICKET', None)
    commit_from_env = os.environ.get('PREVIOUS_RELEASE_COMMIT', None)
    if ticket_from_env and commit_from_env:
        return ticket_from_env, commit_from_env
    if ticket_from_env or commit_from_env:
        raise Exception(
            "Either none or both env variables PREVIOUS_RELEASE_TICKET and PREVIOUS_RELEASE_COMMIT should be defined")

    current_commit_pattern = re.compile(".*CURRENT_FIXTURES_HASH=([a-f0-9]+)", re.DOTALL | re.IGNORECASE)
    ticket = find_previous_ticket()
    key = ticket['key']
    match = current_commit_pattern.match(ticket['description'])
    if not match:
        raise Exception(
            f"Found the previous release ticket {key} but couldn't parse the commit from it")
    return key, match.group(1)


def extract_changelog_tickets(prev_commit, commit):
    changelog = git(f'log {prev_commit}..{commit} -- {yaml_metadata_files}')
    ticket_pattern = re.compile('.*(?:\\b|/\\\\)([A-Z]{3,}-\\d+)\\b.*')
    tickets = ticket_pattern.findall(changelog)
    tickets = list(set(tickets))
    tickets.sort()
    return "\n".join(tickets)


def git(args):
    output = subprocess.check_output(f"git -C {CLOUD_GO_PATH} {args}", shell=True)
    return output.decode()


def dedent(string):
    return re.sub('\n +', '\n', string)


def prepare_release_ticket():
    prev_ticket, prev_commit = find_previous_ticket_and_commit()

    branch = git('branch --show-current').strip()
    commit = git('rev-parse HEAD').strip()
    print(f"previous ticket is {prev_ticket}")
    print(f"cloud-go is on branch '{branch}' and commit '{commit}'")
    changelog = git(f'log --oneline {prev_commit}..{commit} -- {yaml_metadata_files}')

    changelog_tickets = extract_changelog_tickets(prev_commit, commit)
    description = dedent(f"""
        Previous ticket - {prev_ticket}
        ```
        CURRENT_FIXTURES_HASH={commit}
        PREVIOUS_FIXTURES_HASH={prev_commit}
        ```
        <{{commits with changes to `{yaml_metadata_files}` that are included to this release
        ```
        {changelog}
        ```
        }}>
        <{{Ticket list for linking
        {changelog_tickets} 
        }}>
    """)
    url = make_template_url(description, prev_ticket)
    print(f"opening template url in the default browser: {url}")
    webbrowser.open(url)


def make_template_url(description, prev_ticket):
    today = date.today().isoformat()
    query_params = {
        "queue": "CLOUD",
        "description": description,
        "components[]": [
            2369,  # iam
            39829,  # Release Management
        ],
        "type": "12",  # release
        "summary": f"[ {today} ][ Deploy ] IAM : metadata",
        "tags[]": ["iam:duty", "iam-metadata"],
        "linkedIssues[depends on]": [f"{prev_ticket}|ru.yandex.startrek"],
        "assignee": "me()",  # doesn't work like this in url
        "checklistItems[]": [
            '{"text":"day 1: internal-dev"}',
            '{"text":"day 1: YC-testing"}',
            '{"text":"day 1: internal-prestable"}',
            '{"text":"day 1: YC-preprod"}',
            '{"text":"day 1: DC-preprod"}',
            '{"text":"day 2: internal-prod"}',
            '{"text":"day 3: YC-prod node"}',
            '{"text":"day 3: IL node"}',
            '{"text":"day 3: DC-prod"}',
            '{"text":"day 4: YC-prod zone"}',
            '{"text":"day 4: IL all"}',
            '{"text":"day 5: YC-prod all"}',
        ]
    }

    return ST_URL + '/createTicket?' + urlencode(query_params, doseq=True)


CLOUD_GO_PATH = find_cloud_go_path()
prepare_release_ticket()
