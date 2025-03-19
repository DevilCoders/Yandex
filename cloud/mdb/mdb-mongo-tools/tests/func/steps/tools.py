"""
Steps related to mdb-mongo-tools
"""

# pylint: disable=no-name-in-module

from behave import register_type, then, when
from hamcrest import assert_that, contains_string, equal_to
from parse_type import TypeBuilder

from tests.func.helpers import docker, utils
from tests.func.helpers.utils import context_to_dict, render_template

register_type(
    Tool=TypeBuilder.make_enum({
        'resetup': 'mdb-mongod-resetup',
        'stepdown': 'mdb-mongod-stepdown',
        'getter': 'mdb-mongo-get',
    }))


@when('we run {tool:Tool} on {node_name}')
@when('we run {tool:Tool} on {node_name} and exit code is {expected_exit_code:d}')
@when('we run {tool:Tool} on {node_name} with args "{args}"')
@when('we run {tool:Tool} on {node_name} with args "{args}" and exit code is {expected_exit_code:d}')
@then('we run {tool:Tool} on {node_name}')
@then('we run {tool:Tool} on {node_name} and exit code is {expected_exit_code:d}')
@then('we run {tool:Tool} on {node_name} with args "{args}"')
@then('we run {tool:Tool} on {node_name} with args "{args}" and exit code is {expected_exit_code:d}')
@then('we run {tool:Tool} on {node_name} with args "{args}" and exit code is {expected_exit_code:d} and output was')
def step_run_tool(
        context,
        tool,
        node_name,
        args=None,
        expected_exit_code=None,
):
    """
    Run resetup
    """
    if args is None:
        args = ''

    if expected_exit_code is None:
        expected_exit_code = 0

    instance = docker.get_container(context, node_name)
    cmd = utils.strip_query('{tool} {args}'.format(tool=tool, args=args))
    current_exit_code, current_output = docker.exec_run(instance, cmd)

    assert_that(current_exit_code, equal_to(expected_exit_code),
                'Unexpected exit code, command: {cmd}. Output was: {output}'.format(cmd=cmd, output=current_output))

    if context.text is not None:
        templ_data = render_template(context.text, context_to_dict(context))
        assert_that(current_output, contains_string(templ_data))
