from collections.abc import Mapping

import sentry_sdk
import sentry_sdk.integrations.flask

from .config import app_config
from .release import get_release


def _before_send(event, hint):
    if tags := event.get('tags'):
        # According to specification tags is dict or list-of-list
        # https://develop.sentry.dev/sdk/event-payloads/
        if isinstance(tags, Mapping):
            if tags.get('handled') == 'no':
                return event
        elif ['handled', 'no'] in tags:
            return event
    # By default FlaskIntegration capture all errors including handled (client errors)
    # we don't want see them in Sentry
    return None


def init_sentry():
    sentry_config = app_config().get('SENTRY', {})
    sentry_sdk.init(
        dsn=sentry_config.get('dsn'),
        integrations=[sentry_sdk.integrations.flask.FlaskIntegration()],
        traces_sample_rate=sentry_config.get('traces_sample_rate', 0.0),
        release=get_release(),
        environment=sentry_config.get('environment'),
        before_send=_before_send,
    )
