"""
Steps for Apache Livy
"""

from helpers.utils import render_template
import time
import textwrap

from hamcrest import assert_that, not_none, has_key, contains_string, equal_to

from behave import when, then, given

from helpers import livy


@then('stop livy session')
def step_session_stop(ctx):
    """
    Stop livy session from ctx
    """
    resp = livy.session_delete(ctx, ctx.livy_session_id)
    assert_that(resp, has_key('msg'))
    assert_that(resp['msg'], equal_to('deleted'))


@when('start livy session')
@when('start livy session {name}')
@when('start livy {kind} session')
@when('start livy {kind} session {name}')
def step_session_create(ctx, name='example-session', kind='spark'):
    """
    Start livy session with name
    """
    resp = livy.session_create(ctx, session_conf={'kind': kind, 'name': name})
    ctx.livy_session_id = resp['id']


@then('livy session started')
@then('livy session started within {timeout} seconds')
def step_session_created(ctx, timeout=120):
    """
    Check that session created
    """
    start_at = time.time()
    session = {}
    assert_that(ctx.livy_session_id, not_none())
    while time.time() - start_at <= float(timeout):
        session = livy.session_get(ctx, ctx.livy_session_id)
        if session['state'] == 'idle':
            return
        time.sleep(1)
    raise AssertionError(f'Livy session didn\'t started within {timeout} seconds, session {session}')


@given('livy code')
def step_livy_code(ctx):
    """
    Set code for livy statement
    """
    ctx.livy_code = render_template(textwrap.dedent(ctx.text), ctx)


@when('start livy statement')
@when('execute livy statement')
def step_statement_create(ctx):
    """
    Start livy session with name
    """
    livy.statement_create(ctx, ctx.livy_session_id, ctx.livy_code)


@then('livy statement finished')
@then('livy statement finished within {timeout} seconds')
def step_statement_finished(ctx, timeout=120):
    """
    Check that statement finished
    """
    start_at = time.time()
    assert_that(ctx.livy_session_id, not_none())
    assert_that(ctx.livy_code, not_none())

    stmt = livy.statement_create(ctx, ctx.livy_session_id, ctx.livy_code)
    ctx.livy_statement_id = stmt['id']
    while time.time() - start_at <= float(timeout):
        stmt = livy.statement_get(ctx, ctx.livy_session_id, ctx.livy_statement_id)
        if stmt.get('state') == 'available':
            assert_that(stmt, has_key('output'))
            status = stmt['output']['status']
            if status != 'ok':
                raise AssertionError(f'Statement status {status}, traceback: {stmt}')
            ctx.livy_output = stmt['output']['data']['text/plain']
            return
        time.sleep(1)
    raise AssertionError(f'Livy statement didn\'t finished within {timeout}, statement: {stmt}')


@then('livy output contains')
def step_statement_output_contains(ctx):
    """
    Check that statement output contains string
    """
    assert_that(ctx.livy_output, contains_string(ctx.text))
