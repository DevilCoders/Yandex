# coding: utf-8

import yaml

from behave import then, when
import helpers


@when(u'we check lower and upper border of chunk')
def step_check_keyrange(context):
    attrs = yaml.safe_load(context.text)
    connect = context.meta_connects[0]
    context.result = connect.get(
        """
    SELECT upper(chunk), lower(chunk) FROM
        (SELECT s3.to_keyrange(%(start_key)s, %(end_key)s)) f(chunk)
    """,
        start_key=attrs.get('start_key'),
        end_key=attrs.get('end_key'),
    )
    helpers.assert_no_errcode(context)


@then(u'we get following result')
def step_get_result(context):
    print(context.result.records)
    helpers.assert_compare_one_entity_with_result(context)
