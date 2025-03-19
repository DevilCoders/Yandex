import pathlib
import re
import datetime
import requests
import json
import functools
import httmock
from urllib.parse import urlparse, parse_qsl, unquote_plus, urlunparse, urlunsplit, ParseResult, SplitResult
from typing import Iterable

from colorama import Cursor, Style, init as init_colorama

init_colorama()


class Node:
    def __init__(self, issue_id, issue_key):
        self.id = issue_id
        self.key = issue_key
        self.children: list[Node] = []


# обходит поддерево задачи (подзадачи и подэпики) и добавляет узлы в node_by_id
# возвращает Node, соответствующий переданной задаче
def collect_issue_subtree(issue_id, issue_key, node_by_id, get_links_function):
    if issue_id in node_by_id:
        return node_by_id[issue_id]
    node = node_by_id[issue_id] = Node(issue_id, issue_key)
    try:
        links = get_links_function(issue_id)
    except:
        print(f'Cannot load links of {issue_key}')
        return node
    for link in links:
        if (link['type']['id'] == 'epic' and link['direction'] == 'inward') or \
                (link['type']['id'] == 'subtask' and link['direction'] == 'outward'):
            sub_issue_id = link['object']['id']
            sub_issue_key = link['object']['key']
            node.children.append(collect_issue_subtree(sub_issue_id, sub_issue_key, node_by_id, get_links_function))
    return node


def get_or_load_issue_node(nodes_by_id: dict[int, Node], issue, get_links_function) -> Node:
    if issue.id not in nodes_by_id:
        collect_issue_subtree(issue.id, issue.key, nodes_by_id, get_links_function)
    return nodes_by_id[issue.id]


duration_regex = re.compile(r'((?P<days>\d+?)d)?((?P<hours>\d+?)h)?((?P<minutes>\d+?)m)?((?P<seconds>\d+?)s)?')


def parse_duration(time_str):
    parts = duration_regex.match(time_str).groupdict()
    time_params = {}
    for name, param in parts.items():
        if param:
            time_params[name] = int(param)
    return datetime.timedelta(**time_params)


def styled_reprint(*strings, styles=(Style.BRIGHT,), **kwargs):
    styled_print(*strings, styles=(*styles, Cursor.UP()), **kwargs)


def styled_print(*strings, styles=(Style.BRIGHT,), **kwargs):
    sep = kwargs.pop('sep', ' ')
    if not isinstance(styles, Iterable):
        styles = styles,
    print(*styles, sep.join(strings), Style.RESET_ALL, sep='', **kwargs)


class Url(object):
    """
    An url object that can be compared with other url objects
    without regard to the vagaries of encoding, escaping, and ordering
    of parameters in query strings.
    """

    def __init__(self, url):
        if isinstance(url, ParseResult):
            url = urlunparse(url)
        if isinstance(url, SplitResult):
            url = urlunsplit(url)
        parts = urlparse(url)
        _query = frozenset(parse_qsl(parts.query))
        _path = unquote_plus(parts.path)
        self.parts = parts._replace(query=_query, path=_path)

    def __eq__(self, other):
        return self.parts == other.parts

    def __hash__(self):
        return hash(self.parts)

    def __str__(self):
        return self.unparse()

    def unparse(self) -> str:
        query = self.parts.query
        result = urlunparse(
            self.parts._replace(query='&'.join(
                map(lambda k: f'{k[0]}={k[1]}', query))))
        return result


class Logger:
    def __init__(self, log_path):
        self.log_path = log_path
        self.logs = {}

    def __enter__(self):
        self._real_session_send = requests.Session.send

        def _fake_send(session, request: requests.PreparedRequest, **kwargs):
            res = self._real_session_send(session, request=request, **kwargs)
            url = Url(request.url).unparse()
            log = {'json': res.json(), 'headers': {**res.headers}}
            if res.status_code != 200:
                log['statusCode'] = res.status_code
            method = request.method.upper()
            if url not in self.logs:
                self.logs[url] = {}
            if method not in self.logs[url]:
                self.logs[url][method] = {'responses': []}
            if method == 'POST':
                log = {'key': json.loads(request.body), 'response': log}
            if method == 'GET':
                log = {'key': {}, 'response': log}
            self.logs[url][method]['responses'].append(log)
            return res

        requests.Session.send = _fake_send

        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        requests.Session.send = self._real_session_send
        log = pathlib.Path(self.log_path)
        with open(log, 'w' if log.exists() else 'a', encoding='utf8') as file:
            json.dump(self.logs, file, indent=4)


@httmock.all_requests
def mock_with_dict(mocks: dict):

    def wrapper(url, request):
        url = Url(url)
        resp = mocks.get(url)
        if resp is None:
            print('\n', url, 'not in mock_file\n')
            return
        else:
            method = request.method.upper()
            resp = resp.get(method)
        if resp is None:
            print(url, 'is in mock_file, but method is different\n')
            return
        if method == 'POST':
            key = json.loads(request.body)
        elif method == 'GET':
            key = {}
        else:
            key = None
        resp = [i['response'] for i in resp['responses'] if i['key'] == key]
        if len(resp) == 0:
            print(url, f'is in mock_file, key {key} absent\n')
            return
        resp = resp[0]
        content = json.dumps(resp['json'])
        headers = resp.get('headers')
        status_code = resp.get('statusCode', 200)
        return httmock.response(status_code, content=content, headers=headers, request=request)

    return wrapper


def with_mock(mock_file):
    with open(mock_file, 'r', encoding='utf8') as file:
        mocks: dict = {Url(k): v for k, v in json.load(file).items()}

    def decorator(func):
        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            with httmock.HTTMock(mock_with_dict(mocks)):
                return func(*args, **kwargs)

        return wrapper

    return decorator


def with_log(log_file):
    def decorator(func):
        @functools.wraps(func)
        def wrapper(*args, **kwargs):
            with Logger(log_file):
                return func(*args, **kwargs)

        return wrapper

    return decorator
