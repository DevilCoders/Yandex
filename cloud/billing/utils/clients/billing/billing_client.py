import logging
import sys
import uuid

import requests

LOG = logging.getLogger(__name__)
handler = logging.StreamHandler(sys.stdout)
handler.setLevel(logging.DEBUG)
LOG.addHandler(handler)


class BillingClient:
    def __init__(self, endpoint, iam_token):
        self._endpoint = endpoint
        self.__iam_token = iam_token

    def resolve_abc_to_cloud(self, abc_id=None, abc_slug=None):
        request = {}
        if abc_id:
            request["abcId"] = abc_id
        if abc_slug:
            request["abcSlug"] = abc_slug

        # if both set, server will fail on validation
        _, result = self._post("/abc/resolveCloud", request)
        return result.get("cloudId")

    def bind_to_billing_account(self, cloud_id, billing_account_id):
        request = {
            "cloudId": cloud_id,
            "billingAccountId": billing_account_id
        }
        _, result = self._post("/clouds/bind", request)
        return result   # operation

    def _generate_request_id(self):
        return str(uuid.uuid4())

    def _get_headers(self, request_id):
        return {
            "Accept": "application/json",
            "X-Request-ID": request_id,
            "X-YaCloud-SubjectToken": "{}".format(self.__iam_token)
        }

    def _call(self, method, postfix, data=None):
        request_id = self._generate_request_id()
        url = self._endpoint + postfix
        LOG.info("request_id=%s %s url=%s data=%s", request_id, method.capitalize(), url, str(data))
        headers = self._get_headers(request_id)
        response = requests.request(method=method, url=url, headers=headers, json=data)
        LOG.info("request_id=%s url=%s code=%s", request_id, url, response.status_code)
        response.raise_for_status()
        return response.status_code, response.json()

    def _post(self, postfix, data=None):
        return self._call(method="POST", postfix=postfix, data=data)
