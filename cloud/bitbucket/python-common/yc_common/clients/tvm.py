import base64
import hmac
from hashlib import sha256
from time import time
from urllib.parse import urljoin

import requests


class TvmClientError(Exception):
    pass


class TvmClient:
    def __init__(self, tvm_url, client_id, client_secret, timeout=10):
        self.url = tvm_url
        self.client_id = str(client_id)
        self.timeout = timeout
        self.__client_secret = client_secret
        self.__ticket = None
        self.__ticket_expire_time = 0

    def get_ticket(self):
        curr_time = int(time())
        if self.__ticket is None or curr_time > self.__ticket_expire_time:
            ticket = self._fetch_ticket()
            self.__ticket = ticket
            self.__ticket_expire_time = self._get_ticket_expire_time(ticket)
        return self.__ticket

    @staticmethod
    def _get_ticket_expire_time(ticket):
        if not ticket:
            return 0
        try:
            # Reduce actual expire time to avoid ticket expiring during request preparation
            return int(ticket.split(':')[2]) - 10
        except Exception:
            raise ValueError("Invalid ticket: {}.".format(ticket))

    def _fetch_ticket(self):
        ts = int(time())
        ts_sign = self._sign_msg(str(ts))
        data = {
            "ts": ts,
            "client_id": self.client_id,
            "grant_type": "client_credentials",
            "ts_sign": ts_sign,
        }

        resp = requests.post(urljoin(self.url, "/ticket/"), data=data, timeout=self.timeout)
        if resp.status_code == 200:
            ticket = resp.text
            return ticket
        else:
            raise TvmClientError("TVM responded with status {}. Response content: {}.".format(resp.status_code,
                                                                                              resp.content))

    @staticmethod
    def _base64_decode(text):
        if len(text) % 4:
            text += '=' * (4 - len(text) % 4)
        return base64.urlsafe_b64decode(text)

    def _sign_msg(self, msg):
        digest = hmac.new(
            self._base64_decode(self.__client_secret),
            msg=str(msg).encode("utf-8"),
            digestmod=sha256,
        ).digest()
        return base64.urlsafe_b64encode(digest)
