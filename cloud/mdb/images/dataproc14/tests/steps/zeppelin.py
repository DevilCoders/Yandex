"""
Steps for Apache Zeppelin
"""

from hamcrest import assert_that, not_none, has_key, equal_to

from behave import when, then, given
from retrying import retry

from helpers import zeppelin as z


@when('list zeppelin notes')
def step_notebook_list(ctx):
    """
    List all Zeppelin notebooks
    """
    r = z.notebooks_list(ctx)
    assert_that(r, has_key('status'))
    assert_that(r['status'], equal_to('OK'))
    ctx.zpln_notes = r['body']


@when('exists zeppelin note {note}')
def step_notebook_exists(ctx, note):
    """
    List of notes contains {name}
    """
    ctx.zpln_note_id = None
    for notebook in ctx.zpln_notes:
        if notebook['path'] == note:
            ctx.zpln_note_id = notebook['id']
    assert_that(ctx.zpln_note_id, not_none())


@when('list zeppelin paragraphs')
def step_paragraphs_list(ctx):
    """
    List all paragraphs for a note
    """
    r = z.paragraphs_list(ctx, ctx.zpln_note_id)
    assert_that(r, has_key('status'))
    assert_that(r['status'], equal_to('OK'))
    ctx.zpln_paragraphs = r['body']['paragraphs']


@retry(stop_max_attempt_number=5, wait_exponential_multiplier=1000, wait_exponential_max=10000)
def _assert_success_paragraph(ctx, note_id: str, p_id: str):
    """
    Assert that paragraph success
    """
    info = z.paragraph_info(ctx, note_id, p_id)
    assert_that(info, has_key('status'))
    assert_that(info['status'], equal_to('OK'))
    assert_that(info, has_key('body'))
    # Some of paragaphs doesn't have results, its fine.
    if info['body'].get('results'):
        assert_that(info['body']['results'], has_key('code'))
        assert_that(info['body']['results']['code'], equal_to('SUCCESS'))
    assert_that(info['body']['status'], equal_to('FINISHED'))


@then('run zeppelin paragraphs')
def step_paragraphs_run(ctx):
    """
    Sequentially run all paragraphs
    """
    skipped = []
    if hasattr(ctx, 'zpln_paragraphs_skip'):
        skipped = ctx.zpln_paragraphs_skip
    for p in ctx.zpln_paragraphs:
        if skipped:
            if p['id'] in skipped:
                continue
        run = z.paragraph_run(ctx, ctx.zpln_note_id, p['id'])
        assert_that(run, has_key('status'))
        assert_that(run['status'], equal_to('OK'))
        _assert_success_paragraph(ctx, ctx.zpln_note_id, p['id'])


@given('skip paragraph {paragraph_id}')
@given('skip zeppelin paragraph {paragraph_id}')
def step_skip_zeppelin_paragraph(ctx, paragraph_id):
    """
    Skip some of paragraphs
    """
    if not hasattr(ctx, 'zpln_paragraphs_skip'):
        ctx.zpln_paragraphs_skip = []
    ctx.zpln_paragraphs_skip.append(paragraph_id)


@then('run zeppelin note {note}')
def step_run_zeppelin_note(ctx, note):
    """
    Ensure that note exists and run all paragraphs
    """
    step_notebook_list(ctx)
    step_notebook_exists(ctx, note)
    step_paragraphs_list(ctx)
    step_paragraphs_run(ctx)
