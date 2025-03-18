"""
    Staff related functions, like get list of users by department, convert groups to users, etc...
"""

import urllib
import urllib2
import ssl
import simplejson

import config
from gaux.aux_utils import retry_urlopen, retry_requests_get
from gaux.aux_decorators import decayed_memoize
from gaux.aux_abc import unwrap_abc_group


class TStaffUser(object):
    __slots__ = ['name', 'description']

    def __init__(self, data):
        self.name = data["login"]
        self.description = "%s %s" % (data["name"]["first"]["en"], data["name"]["last"]["en"])


class TStaffDepartment(object):
    __slots__ = ['name', 'description']

    def __init__(self, data):
        self.name = data["url"]
        self.description = data["name"]


@decayed_memoize(120)
def run_staff_query(path, cgi):
    """
        Requst staff rest api.

        :param path: path in request, like </v3/persons> or </v3/groups>
        :param cgi: list of pairs (cgi_name, cgi_value)

        :return: response json
    """
    from core.settings import SETTINGS  # have to import here to avoid circular import

    cgi_line = "&".join(map(lambda (name, value): "%s=%s" % (name, urllib.quote(value)), cgi))
    url = "%s%s?%s" % (SETTINGS.services.staff.rest.v1.url, path, cgi_line)
    headers = {
        "Authorization": "OAuth %s" % config.get_default_oauth(),
    }

    req = urllib2.Request(url, None, headers)

    try:
        resp = simplejson.loads(retry_urlopen(5, req, None, 3.0))
    except (urllib2.HTTPError, ssl.SSLError) as e:
        raise Exception("Got exception <%s> while processing url <%s>" % (e.__class__, url))

    return resp


def get_dpt_users(dpt):
    """
        Function to get users from staff. Can be slow (up to 0.5 seconds) and should be somehow cached

        :type dpt: str

        :param dpt: deparatament name without dpt_ prefix, e. g. yandex_mnt, yandex, ...
        :return (list of TStaffUser): list of login names
    """

    cgi_data = (
        ("_query", 'department_group.department.url=="%s" or department_group.ancestors.department.url=="%s"' % (dpt, dpt)),
        ("official.is_dismissed", "false"),
        ("_fields", "login,name"),
        ("_limit", "30000"),
    )
    resp = run_staff_query("/v3/persons", cgi_data)
    users = map(lambda x: TStaffUser(x), resp["result"])

    return users


@decayed_memoize(120)
def get_dpts():
    """
        Return list of departments, as department can be used as group owner
    """

    cgi_data = (
        ('_query', 'type == "department"'),
        ('_limit', '10000'),
    )
    resp = run_staff_query("/v3/groups", cgi_data)
    departments = map(lambda x: TStaffDepartment(x), resp["result"])

    return departments


@decayed_memoize(120)
def get_possible_group_owners():
    """
        Return list of users and groups, which can be used as owners in group card.
    """
    return map(lambda x: x.name, get_dpts()) + \
           map(lambda x: x.name, get_dpt_users("yandex")) + \
           map(lambda x: x.name, get_dpt_users("ext")) + \
           map(lambda x: x.name, get_dpt_users("virtual")) + \
           ['c1r1aas', 'test_owner1', 'test_owner2']  # add some special users


def unwrap_dpts(users, suppress_missing=False, db=None):
    """Convert list of users with replacing dpts by list of dpts users"""
    if db is None:
        from core.db import CURDB
        db = CURDB

    if db.version < '2.2.48':
        all_dpts = {x.name for x in get_dpts()}

        users = set(users)
        dpts = users & all_dpts
        users = users - all_dpts
        for dpt in dpts:
            dpt_users = {x.name for x in get_dpt_users(dpt)}
            users |= set(dpt_users)

        return sorted(list(users))
    else:  # RX-474
        result = set()
        for user_or_group in users:
            if user_or_group.startswith('svc_'):  # abc groups in hbf macroses are presented as svc_<abcgroup>
                user_or_group = 'abc:{}'.format(user_or_group[4:])

            if user_or_group.startswith('abc:'):  # RX-447
                for username in unwrap_abc_group(user_or_group):
                    result.add(username)

            elif db.staffgroups.has(user_or_group):
                for user in db.staffgroups.get(user_or_group).recurse_users:
                    result.add(user.name)
            else:
                result.add(user_or_group)

        return sorted(list(result))


def list_staff_users():
    """Get all staff users with group they belongs to and fire time (RX-474)"""
    from core.settings import SETTINGS
    url = '{}/persons?_fields=login,official.quit_at,official.is_dismissed,department_group.department.url&_limit=100000'.format(SETTINGS.services.staff.rest.v3.url)
    headers = {
        'Authorization': 'OAuth {}'.format(config.get_default_oauth()),
        'Accept': 'application/json',
    }

    result = []
    while True:
        response = retry_requests_get(5, url, headers=headers, allow_redirects=False, verify=False)
        if response.status_code == 200:
            response_json = response.json()
            for elem in response_json['result']:
                if elem['official']['is_dismissed']:
                    retired_at = elem['official']['quit_at']
                else:
                    retired_at = None
                result.append(dict(name=elem['login'], staff_group=elem['department_group']['department']['url'], retired_at=retired_at))

            if response_json['links'].get('next', None) is not None:
                url = response_json['links']['next']
            else:
                break
        else:
            raise Exception('Request <{}> returned status code <{}>'.format(url, response.status_code))

    return result


def list_staff_groups():
    """Get all staff groups to create tree structure (RX-474)"""
    from core.settings import SETTINGS
    url = '{}/groups?type=department&_fields=department.url,ancestors.department.url&_limit=100000'.format(SETTINGS.services.staff.rest.v3.url)
    headers = {
        'Authorization': 'OAuth {}'.format(config.get_default_oauth()),
        'Accept': 'application/json',
    }

    result = []
    while True:
        response = retry_requests_get(5, url, headers=headers, allow_redirects=False, verify=False)
        if response.status_code == 200:
            response_json = response.json()
            for elem in response_json['result']:
                if len(elem['ancestors']):
                    parent_group = elem['ancestors'][-1]['department']['url']
                else:
                    parent_group = None

                result.append(dict(name=elem['department']['url'], parent_group=parent_group))

            if response_json['links'].get('next', None) is not None:
                url = response_json['links']['next']
            else:
                break
        else:
            raise Exception('Request <{}> returned status code <{}>'.format(url, response.status_code))

    return result
