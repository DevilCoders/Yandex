# coding: utf-8

from __future__ import unicode_literals

from django.conf import settings
from ids.helpers import oauth as ids_oauth


def get_token_by_sessionid(*args, **kwargs):
    return ids_oauth.get_token_by_sessionid(
        settings.OAUTH_ID,
        settings.OAUTH_SECRET,
        *args,
        **kwargs
    )


def get_token_by_password(*args, **kwargs):
    return ids_oauth.get_token_by_password(
        settings.OAUTH_ID,
        settings.OAUTH_SECRET,
        *args,
        **kwargs
    )


def get_token_by_uid(*args, **kwargs):
    # СИБ запрещает получать токены таким образом
    return 'bad_token_by_uid'
    # return ids_oauth.get_token_by_uid(
    #     settings.OAUTH_ID,
    #     settings.OAUTH_SECRET,
    #     *args,
    #     **kwargs
    # )
