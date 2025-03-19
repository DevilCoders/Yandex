#!/usr/bin/env python
# -*- coding: utf-8 -*-

import json
import logging
import time
import zlib

import psycopg2
from psycopg2.extensions import make_dsn
import requests
import yaml
from hamcrest import assert_that, equal_to, has_entry, matches_regexp

from matcher import iterable_contains

LOG = logging.getLogger(__name__)


@given(u'a deployed DBM')
def step_deployed_dbm(context):
    context.execute_steps(
        u"""
        When we issue "GET /ping" request with 10 retries
        Then we get response with code 200
    """
    )


def get_dbmdb_dsn(context, user=None):
    dsn_vars = {
        'host': context.dbmdb_host,
        'port': context.dbmdb_port,
        'dbname': 'dbm',
    }
    if user:
        dsn_vars['user'] = user
    dsn = make_dsn(**dsn_vars)
    return dsn


# pylint: disable=unused-argument
@given(u'empty DB')
def step_empty_db(context):
    """
    Clears some tables in DB
    """
    conn = psycopg2.connect(get_dbmdb_dsn(context))
    with conn as txn:
        cur = txn.cursor()
        cur.execute('TRUNCATE TABLE mdb.volume_backups')
        cur.execute('TRUNCATE TABLE mdb.volumes')
        cur.execute('TRUNCATE TABLE mdb.containers CASCADE')
        cur.execute('TRUNCATE TABLE mdb.dom0_hosts CASCADE')
        cur.execute('TRUNCATE TABLE mdb.clusters CASCADE')
        cur.execute('TRUNCATE TABLE mdb.projects CASCADE')
        cur.execute('TRUNCATE TABLE mdb.locations CASCADE')
        cur.execute('INSERT INTO mdb.projects ' "(name, description) VALUES ('mdb', ''), ('noresources', '')")
        cur.execute("INSERT INTO mdb.locations (geo) VALUES ('vla')")
    conn.close()


@when(u'we run query')
def step_db_query(context):
    """
    Run query in dbmdb
    """
    conn = psycopg2.connect(get_dbmdb_dsn(context))
    with conn as txn:
        cur = txn.cursor()
        cur.execute(context.text)
    conn.close()


@when(u'we lock {geo:w}')
def step_lock_geo(context, geo):
    conn = psycopg2.connect(get_dbmdb_dsn(context, 'dbm'))
    cur = conn.cursor()
    cur.execute('SELECT pg_advisory_lock(%(geo_id)s)', {'geo_id': zlib.crc32(geo.encode('utf-8'))})
    if 'open_connections' not in context:
        context.open_connections = []
    context.open_connections.append(conn)


@when(u'we issue "{method} {request}" request with {retries:d} retries')
def step_issue_request_with_retry(context, method, request, retries):
    """
    Issues HTTP request with retries and writes request object into context
    """
    tries = 0
    last_exc = None
    while tries < retries:
        try:
            res = issue_http_request(context.dbm_port, method, request)
            res.raise_for_status()
            context.request = res
            return
        except Exception as exc:
            last_exc = exc
            time.sleep(1)
        tries += 1
    raise AssertionError(f'{retries} did not help. Last error: {last_exc}')


@when('we issue "{method} {request}" request on metrics endpoint')
def step_issue_request_on_metrics(context, method, request):
    res = issue_http_request(context.dbm_metrics_port, method, request)
    res.raise_for_status()
    context.request = res


@when(u'we issue "{method} {request}" with')
def step_issue_request_with(context, method, request):
    """
    Issues HTTP request and writes request object into context
    """
    src = yaml.safe_load(context.text)
    timeout = src['timeout'] if 'timeout' in src else 2
    headers = src['headers'] if 'headers' in src else None
    body = json.dumps(src['body']) if 'body' in src else None

    context.request = issue_http_request(context.dbm_port, method, request, headers=headers, body=body, timeout=timeout)


@then(u'we get response with code {code:d}')
def then_response_with_code(context, code):
    """
    Checks HTTP response code
    """
    req = context.request
    LOG.info(req.status_code)
    assert_that(req.status_code, equal_to(code), f'Got body: {req.text}')


@then(u'body is')
def step_body_is(context):
    body = yaml.load(context.text)
    req = context.request
    LOG.info(req.text)
    result = req.json()
    assert_that(result, equal_to(body))


@then(u'body contains line starts with')
def step_body_is(context):
    req = context.request
    LOG.info(req.text)
    for line in context.request.text.split('\n'):
        if line.startswith(context.text.strip()):
            return
    raise AssertionError(f'{context.text} line not found in {req.text}')


@then(u'body contains')
def step_body_contains(context):
    """
    Helper for checking json documents
    """
    body = yaml.load(context.text)
    req = context.request
    LOG.info(req.text)
    result = req.json()
    if isinstance(body, (dict, list)):
        assert_that(result, iterable_contains(body))
    else:
        raise RuntimeError('Body is of unexpected type {t}'.format(t=type(body)))


@then(u'body contains error matches')
def step_body_contains_error_matches(context):
    """
    Helper for checking error matches regexp
    """
    req = context.request
    LOG.info(req.text)
    result = req.json()
    assert_that(result, has_entry('error', matches_regexp(context.text)))


def issue_http_request(port, method, request, headers=None, body=None, timeout=2):
    """
    Issues HTTP request and returns request object
    """
    url = 'http://localhost:{port}{request}'.format(port=port, request=request)

    kwargs = {'allow_redirects': False, 'timeout': timeout}
    if headers:
        kwargs['headers'] = headers

    if method == 'GET':
        return requests.get(url, **kwargs)
    elif method == 'POST':
        LOG.info(body)
        return requests.post(url, data=body, **kwargs)
    elif method == 'PUT':
        LOG.info(body)
        return requests.put(url, data=body, **kwargs)
    elif method == 'DELETE':
        LOG.info(body)
        return requests.delete(url, data=body, **kwargs)
    else:
        raise RuntimeError('Not supported yet')
