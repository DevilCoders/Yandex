"""
Steps related to Fake SMTP
"""
from behave import then
from hamcrest import assert_that, empty, has_key, is_not

from tests.helpers import fake_smtp


@then('user "{login}" got email with new password')
def step_check_user_inbox(context, login):
    messages = fake_smtp.get_messages(context)
    email = '{}@yandex-team.ru'.format(login)
    assert_that(messages, has_key(email))
    assert_that(messages[email], is_not(empty()))
