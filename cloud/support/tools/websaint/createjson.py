"""
Dirty script to fetch quota approval rules from wiki.
Usage:
    $ OAUTH_TOKEN="<INSERT_OAUTH_TOKEN_HERE>" python fetch_quota_rules.py >quota_rules.json
Make sure to check exit code. May fail ugly.
Instructions on getting token: https://wiki.yandex-team.ru/wiki/dev/api/autodocs/#autentifikacija
The application shouldd have wiki read access. And It's better to use zombie account
If requests library complains about self-signed certificate, please run add_yandex_ca.py script in the same venv.
"""
import requests
from requests.packages.urllib3.exceptions import InsecureRequestWarning
from datetime import datetime

JSONFILENAME = "./static/limits.js"
TXTFILENAME = "./static/bad.txt"
LOG = "./log/createjson.log"
DEFAULT_APPROVERS = ["prolog", "theigel"]
QUOTAS_GRID_URL = 'https://wiki.yandex-team.ru/cloud/compute/capacity/duty/quotas/autoapprove'
BADLIST_GRID_URL = 'https://wiki.yandex-team.ru/users/the-nans/badlist'


def get_duty(service):
    """
    Return list of current on-call engeniers for the `service'.
    https://wiki.yandex-team.ru/cloud/devel/gore/
    """
    DUTY_URL_TEMPLATE = 'https://resps-api.cloud.yandex.net/api/v0/duty/{service}?from={date_from}&to={date_to}'

    date_str = datetime.now().strftime('%d-%m-%YT%H:%M')
    url = DUTY_URL_TEMPLATE.format(service=service, date_from=date_str, date_to=date_str)
    resp = requests.get(url, verify=False)
    if resp.status_code != 200:
        raise ValueError('Unexpected error: {status}. {error}'.format(status=resp.status_code, error=resp.text))
    duty = list(filter(None, (row.get("resp", {}).get("username") for row in resp.json())))

    return duty


def strip_prefix(s, prefix):
    if s.startswith(prefix):
        return s[len(prefix):]
    return s


def fetch_wiki_grid(url, oauth_token):
    """
    Fetch grid from wiki
    """
    GRID_URL_TEMPLATE = 'https://wiki-api.yandex-team.ru/_api/frontend{path}/.grid?format=json'

    url = strip_prefix(url, 'https://')
    url = strip_prefix(url, 'wiki.yandex-team.ru')
    url = url.rstrip('/')
    if not url.startswith('/'):
        url = '/' + url

    resp = requests.get(GRID_URL_TEMPLATE.format(path=url), headers={'Authorization': 'OAuth ' + oauth_token})

    if resp.status_code != 200:
        raise ValueError('Unexpected error: {status}. {error}'.format(status=resp.status_code, error=resp.text))
    data = resp.json()["data"]
    structure = data["structure"]
    rows = data["rows"]

    fields = [f["title"] for f in structure["fields"]]
    result = []
    for row in rows:
        result.append([rec["raw"] for rec in row])

    return result


def parse_number(value, quota_name=None, default=0):
    value = value.lower().strip().strip('!')
    if value == 'unlimited':
        int_value = float('inf')   # any increase is allowed
    elif not value:
        # empty value means that no rules are specified, need to always summon responsible people
        # default may be specified if the field is an override
        int_value = default
    else:
        try:
            int_value = int(value.rstrip('gb%'))
        except ValueError:
            import sys
            sys.stderr.write('Cannot parse percent for quota {quota_name}: {value}\n'.format(quota_name=quota_name, value=value))
            sys.exit(1)
    return int_value


def fetch_quota_rules(oauth_token):
    duty_cache = {}
    rules = fetch_wiki_grid(url=QUOTAS_GRID_URL, oauth_token=oauth_token)
    rules_enriched = {}
    for row in rules:
        #quota_name, base_limit_text, enterprise_limit_text, safety_limit_text, approvers, comment = row
        quota_name, base_limit_text, enterprise_limit_text, safety_limit_text, hidden, approvers, comment = row[:7]
        quota_name = quota_name.strip()
        safety_limit = parse_number(safety_limit_text, quota_name=quota_name)
        base_limit = parse_number(base_limit_text, quota_name=quota_name)
        enterprise_limit = parse_number(enterprise_limit_text, quota_name=quota_name, default=safety_limit)
        comment = comment.strip().split()
        if len(comment) > 1 and comment[0] == '/duty':
            team = comment[1]
            onduty = get_duty(team)
            approvers.extend(onduty)
        if not approvers:
            approvers = DEFAULT_APPROVERS
        rules_enriched[quota_name] = {
            "safety_limit": safety_limit,
            "base_limit": base_limit,
            "enterprise_limit": enterprise_limit,
            "approvers": approvers,
        }

    return rules_enriched

def fetch_badlist(oauth_token):
    bad = fetch_wiki_grid(url=BADLIST_GRID_URL, oauth_token=oauth_token)
    result = []
    for item in bad:
        result.append(''.join(item).split('\n')[0])
    return result


def main():
    import json, os, sys
    logrecord = ""
    oauth_token = ""
    rules = fetch_quota_rules(oauth_token)
    badlist = fetch_badlist(oauth_token)

    try:
        open(JSONFILENAME, 'w').write("limits = " + json.dumps(rules))
        logrecord = '{when} limits updated \n'.format(when=datetime.now().strftime('%d-%m-%YT%H:%M'))
    except BaseException as e:
        logrecord = '{when} {e}'.format(when=datetime.now().strftime('%d-%m-%YT%H:%M'),e=e)
    finally:
        open(LOG, 'a').write(logrecord)

    try:
        with open(TXTFILENAME, 'w') as handle:
            handle.writelines('\n'.join(badlist))
        logrecord = '{when} badlist updated \n'.format(when=datetime.now().strftime('%d-%m-%YT%H:%M'))
    except BaseException as e:
        logrecord = '{when} {e}'.format(when=datetime.now().strftime('%d-%m-%YT%H:%M'),e=e)
    # finally:
       # open(LOG, 'a').write(logrecord)

if __name__ == '__main__':
    requests.packages.urllib3.disable_warnings(InsecureRequestWarning)
    main()
