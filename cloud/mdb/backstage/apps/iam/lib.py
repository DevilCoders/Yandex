from django.conf import settings

import metrika.pylib.http


def get_access_token(code):
    response = metrika.pylib.http.request(
        method='POST',
        url=f'{settings.CONFIG.iam.oauth_get_token_url}?grant_type=authorization_code&code={code}',
        auth=(settings.CONFIG.iam.client_id, settings.CONFIG.iam.client_secret),
    )
    return response.json()['access_token']
