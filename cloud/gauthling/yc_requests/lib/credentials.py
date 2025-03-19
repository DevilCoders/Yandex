# coding: utf-8

import requests
import yc_requests.sessions


class YandexCloudCredentials:
    def __init__(self, token, secret_key):
        self.token = token
        self.secret_key = secret_key

    @classmethod
    def from_oauth_token(cls, identity_url, oauth_token, organization_id, api_prefix="public/v1alpha1",
                         session=None):
        auth_url = identity_url.rstrip('/') + "/" + api_prefix + "/auth/oauth"

        headers = {
            "User-Agent": "yc-requests",
            "Authorization": "OAuth {}".format(oauth_token),
        }

        data = {"organizationId": organization_id}

        if session is None:
            session = requests.session()
            session.verify = True

        send_kwargs = yc_requests.sessions.merged_session_env(session, auth_url)
        req = requests.Request(method="POST", url=auth_url, headers=headers, json=data)
        resp = session.send(req.prepare(), **send_kwargs)

        if resp.status_code != 200:
            try:
                error_message = resp.json()
            except ValueError:
                error_message = resp.text
            raise requests.HTTPError("{} {}: {}".format(resp.status_code, resp.reason, error_message))

        resp_body = resp.json()

        # FIXME: Support snake_case and camelCase for now (see https://st.yandex-team.ru/CLOUD-5939)
        secret_key = resp_body.get("secretKey", resp_body.get("secret_key"))
        if secret_key is None:
            raise ValueError("could not find neither secretKey nor secret_key in Identity response")

        return cls(token=resp_body["token"], secret_key=secret_key)


AuthCredentialTypes = YandexCloudCredentials
