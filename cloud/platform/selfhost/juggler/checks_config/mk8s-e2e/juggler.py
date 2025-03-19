#!/usr/bin/env python2
# -*- coding: utf-8 -*-

import sys

from juggler_sdk import Check, NotificationOptions

######################
# Checks generation
######################

namespace = "ycloud.mk8s.e2e"
service_common_tag = "yc-mks"


def host_from_env(env):
    return "cloud_{}_mk8s_e2e".format(env)


def required_tags(env):
    return [
        "_".join([service_common_tag, "e2e"]),
        "_".join([service_common_tag, "e2e", env]),
    ]


def telegram_notification(notification_login):
    return NotificationOptions(
        # Report on description change. That is, report every failed build.
        template_name="on_desc_change",
        template_kwargs={
            # No OKs. That is, do not report when test passes after fail.
            "status": ["WARN", "CRIT"],
            "login": [notification_login],
            "method": ["telegram"],
            "delay": "0",
        },
    )


# k8s_alerts_telegram_chat_id = "k8s-alerts"
env_notifications = {
    "prod": [telegram_notification("k8s-alert-prod-e2e")],
    "preprod": [telegram_notification("k8s-alerts-preprod")],
    "dev": [telegram_notification("k8s-alerts-preprod")],
}
hour = 60 * 60
controlplane_run_period = hour


def check(
    env,
    service,
    notifications=None,
    # In juggler cod `ttl` recommended to be 3x of raw event refresh interval.
    ttl=3 * controlplane_run_period,
    **kwargs
):
    """
    :param notifications: список нотификаций
    :type notifications: list[juggler_sdk.NotificationOptions]
    """
    if env not in ["prod", "preprod", "dev"]:
        raise Exception("unexpected env: {}".format(env))
    if service == "":
        raise Exception("service required")

    if notifications is None:
        notifications = env_notifications[env]

    return Check(
        host=(host_from_env(env)),
        service=service,
        namespace=namespace,
        notifications=notifications,
        tags=required_tags(env),
        ttl=ttl,
        **kwargs
    )


def get_checks():
    checks = [
        check('dev', 'master', ttl=9 * hour),
    ]

    for env in ['preprod', 'prod']:
        checks.append(check(env, 'update'))
        for kind in ['controlplane', 'dataplane']:
            for channel in ['stable', 'regular', 'rapid']:
                service = kind + '-' + channel
                checks.append(check(env, service))

    return checks


class Generator(object):
    def __init__(self, args):
        pass

    @staticmethod
    def add_arguments(parser):
        pass

    def filename(self):
        return 'checks'

    def checks(self):
        return get_checks()


if __name__ == '__main__':
    sys.path.append('..')
    from juggler_cli import run_cli

    run_cli(Generator)
