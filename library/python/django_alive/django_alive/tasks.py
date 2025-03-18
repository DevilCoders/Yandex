# coding: utf-8

from celery import shared_task

from .loading import get_check


@shared_task
def check(name, group, host=None):
    checker = get_check(name)

    checker.touch(group, host)
