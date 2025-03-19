"""
Helpers for working with yandexcloud sdk
"""

import os
import sh
import logging


from vault_client.instances import Production

LOG = logging.getLogger('vault')
exception_text = """
Please, specify yav_oauth token for yav.yandex-team.ru as env variable or config property.
https://oauth.yandex-team.ru/authorize?response_type=token&client_id=ce68fbebc76c4ffda974049083729982
"""


def _get_vault(ctx):
    """
    Initialize yandex vault client if not exists
    """
    v = ctx.state.get('vault')
    if v:
        return v
    token = os.getenv('yav_oauth', ctx.conf.get('yav_oauth'))
    if not token:
        out = sh.ya('vault', 'oauth')
        token = out.strip()
    if not token:
        raise Exception(exception_text)
    v = Production(authorization=f'Oauth {token}')
    ctx.state['vault'] = v
    return v


def get_version(ctx, secret_id, **kwargs):
    """
    Get latest version of secret_id
    """
    v = _get_vault(ctx)
    return v.get_version(secret_id, **kwargs)
