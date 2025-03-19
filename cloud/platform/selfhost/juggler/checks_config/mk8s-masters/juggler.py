#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys

from juggler_sdk import Check, NotificationOptions, Child, FlapOptions

######################
# Checks generation
######################

namespace = "ycloud.mk8s.client-masters"
common_tag = "yc-mks"
selfhost_cgroup_suffix = "mk8s-masters"


def cgroup_from_env(env):
    return "cloud_{}_{}".format(env, selfhost_cgroup_suffix)


def required_tags(env):
    return [
        common_tag,
        "_".join([common_tag, env]),
        "_".join([common_tag, env, selfhost_cgroup_suffix]),
    ]


def telegram_notification(chat_id):
    return NotificationOptions(
        template_name="on_status_change",
        template_kwargs={
            "status": ["OK", "WARN", "CRIT"],
            "login": [chat_id],
            "method": ["telegram"],
            "delay": "60",
        },
    )


env_notifications = {
    "prod": [telegram_notification("k8s-alerts")],
    "preprod": [telegram_notification("k8s-alerts-preprod")],
    "gpn": [telegram_notification("k8s-alerts")],
}


def custom_check(
        env,
        service,
        aggregator,
        notifications=None,
        refresh_time=120,  # Makes sense only for active checks. Nop for passive.
        # In juggler cod `ttl` recommended to be 3x of raw event refresh interval.
        # Raw events refreshed:
        # For passive checks - every `interval` configured for check in bundle. 5min by default.
        # For active checks - every `refresh_time` says juggler doc, but 2-6 min in real. See https://st.yandex-team.ru/CLOUD-30531#5dfa632cc232636d059c6ed8
        # So, let ttl be 16 min, which is 3x + delta for raw event refresh interval for now.
        # TODO(skipor): should be decreased after passive check interval decrease in https://st.yandex-team.ru/CLOUD-35404
        ttl=16 * 60,
        flaps_config=FlapOptions(
            stable=12 * 60,  # 2x default passive check interval (5m) + 2m delta.
            critical=60 * 60,  # 5x stable time.
        ),
        **kwargs
):
    """
    :param notifications: список нотификаций, дефолтно пустой
    :type notifications: list[juggler_sdk.NotificationOptions]
    :type aggregator: dict
    """
    if env not in ["prod", "preprod", "gpn"]:
        raise Exception("unexpected env: {}".format(env))
    if service == "":
        raise Exception("service required")
    if notifications is None:
        notifications = env_notifications[env]
    cgroup = cgroup_from_env(env)

    kwargs.update(aggregator)

    return Check(
        host=cgroup,
        service=service,
        namespace=namespace,
        notifications=notifications,
        tags=required_tags(env),
        children=[Child(service=service, host=cgroup, group_type="CGROUP")],
        ttl=ttl,
        refresh_time=refresh_time,
        flaps_config=flaps_config,
        **kwargs
    )

# for anything but UNREACHABLE to suppress when UNREACHABLE or passive-check-deliver is crit
# https://wiki.yandex-team.ru/sm/juggler/check-dependencies/
permanent_limits_normal_1_crit_unreach_or_deliver_force_ok = {
    "aggregator": "more_than_limit_is_problem",
    "aggregator_kwargs": {
        # TODO(skipor): remove after system NO DATA fix
        "nodata_mode": "force_ok",
        "mode": "normal",
        "crit_limit": 1,
        "warn_limit": 100,
        "unreach_mode": "force_ok",
        "unreach_service": [
            {
                "check": ":UNREACHABLE",
                "hold": 600,
            },
            {
                # NOTE(skipor): this is not works, in most noisy case, when juggler see host with conductor sync delay,
                #  because passive-check-deliver set to always run in juggler bundle and runs more ofter than other
                #  checks, so it's OK when juggler see host, and other passive checks are NO DATA,
                #  because they run by juggler-client only when juggler know about that host.
                #  But this works for cases, when juggler client is down,
                #  and allows to get only NO DATA for passive-check-deliver.
                "check": ":passive-check-deliver",
                "hold": 600,
            },
        ],
    },
}


def get_checks(env):
    def check(service,
              aggregator=permanent_limits_normal_1_crit_unreach_or_deliver_force_ok,
              **kwargs):
        return custom_check(
            env, service, aggregator,
            **kwargs
        )

    # UNREACHABLE and ssh
    active_checks_flaps_config = FlapOptions(
        # The Actual refresh interval for active checks is 8 min.
        # Large enough stable_time covers flaps because of a compute node fails.
        stable=24 * 60,
        # Consider the host available just when an OK check happens.
        boost=1,
        # It seems that critical time makes no sense, when boost time is less than refresh interval.
        critical=0,
    )
    checks = [
        check(
            "UNREACHABLE",
            active="icmpping",
            aggregator={
                "aggregator": "more_than_limit_is_problem",
                "aggregator_kwargs": {
                    "nodata_mode": "force_ok",  # TODO(skipor): remove after system NO DATA fix
                    "mode": "normal",
                    "crit_limit": 1,
                    "warn_limit": 100,
                },
            },
            flaps_config=active_checks_flaps_config
        ),
        check(
            "ssh",
            active="ssh",
            active_kwargs={"timeout": 10},
            flaps_config=active_checks_flaps_config
        ),
        custom_check(env, "passive-check-deliver", {
            "aggregator": "more_than_limit_is_problem",
            "aggregator_kwargs": {
                # Check runs always, so there should be no "expected NO DATA", so should live
                "nodata_mode": "force_crit",
                "mode": "normal",
                "crit_limit": 1,
                "warn_limit": 100,
                "unreach_mode": "force_ok",
                "unreach_service": [
                    {
                        "check": ":UNREACHABLE",
                        "hold": 600,
                    },
                ],
            },
        }),
        check("oom-killer"),
        check("freespace"),
        check("reboot-count"),
        check("coredumps"),
        check("k8s-master-authnwebhook"),
        check("k8s-master-yandex-ccm"),
        check("k8s-master-csi-controller"),
        check("k8s-master-csi-provisioner"),
        check("k8s-master-csi-attacher"),
        check("k8s-master-csi-liveness-probe"),
    ]
    return checks


class Generator(object):
    def __init__(self, args):
        self.env = args.env
        pass

    @staticmethod
    def add_arguments(parser):
        parser.add_argument("--env", dest="env", choices=['prod', 'preprod', 'gpn'], required=True,
                            help='Cloud environment.')

    def filename(self):
        return self.env

    def checks(self):
        return get_checks(self.env)


if __name__ == '__main__':
    sys.path.append('..')
    from juggler_cli import run_cli

    run_cli(Generator)
