"""
See https://docs.yandex-team.ru/blackbox/concepts/intro-api
"""
import logging
from collections import namedtuple

import requests
from flask import current_app, request
from retry import retry
from urllib3.exceptions import TimeoutError
from tvmauth import (
    BlackboxEnv,
)
from tvmauth .exceptions import (
    TvmException,
)


BLACKBOX_TIMEOUT_SEC = 3


BlackboxType = namedtuple("BlackboxType", ["tvm_client_id", "url", "env"])


def get_user_ip():
    userip = request.headers.get("X-Real-Ip")
    if not userip:
        x_forwarded_for = request.headers.get("X-Forwarded-For")
        if x_forwarded_for:
            userip = [ip.strip() for ip in x_forwarded_for.split(",", 1)][0]
        else:
            userip = request.remote_addr
    return userip


class Blackbox(object):
    Prod = BlackboxType(222, "http://blackbox.yandex.net/blackbox", BlackboxEnv.Prod)
    Mimino = BlackboxType(239, "http://blackbox-mimino.yandex.net/blackbox", BlackboxEnv.Prod)
    Stub = BlackboxType(239, "http://blackbox.common.yandex.ru/blackbox", BlackboxEnv.Prod)


class BlackboxException(BaseException):
    pass


class BlackboxFatalException(BlackboxException):
    pass


class BlackboxRetryableException(BlackboxException):
    pass


class BlackboxClient(object):
    def __init__(self, blackbox, context):
        self.blackbox = blackbox
        self.context = context

    def get_user_ticket(self, session_id):
        response = self._make_blackbox_get_request(format="json",
                                                   method="sessionid",
                                                   host="yandex.ru",
                                                   sessionid=session_id,
                                                   get_user_ticket="yes",
                                                   userip=get_user_ip())
        ticket = response.get('user_ticket')
        if ticket is None:
            current_app.logger.error("Blackbox responded with no user ticket", extra=dict(response=response))
            raise BlackboxException("Blackbox didn't return user ticket")
        return ticket

    def get_user_logins(self, user_ids):
        if not user_ids:
            return {}

        response = self._make_blackbox_get_request(userip=get_user_ip(),
                                                   format="json",
                                                   method="userinfo",
                                                   uid=", ".join(map(str, user_ids)),
                                                   host="yandex.ru")

        return {int(u["id"]): u["login"] for u in response["users"]}

    def get_user_ids(self, user_logins):
        if not user_logins:
            return {}
        responce = self._make_blackbox_get_request(userip=get_user_ip(),
                                                   format="json",
                                                   method="userinfo",
                                                   login=", ".join(user_logins),
                                                   host="yandex.ru")

        return {u["login"]: int(u["id"]) for u in responce["users"]}

    @retry(tries=2, exceptions=BlackboxRetryableException, logger=logging.getLogger("flask.app"))
    def _make_blackbox_get_request(self, method, **params):
        try:
            ticket_for_blackbox = self.context.tvm.get_service_ticket_for("blackbox")
        except TvmException:
            current_app.logger.exception("Fetching ticket for blackbox failed")
            raise BlackboxRetryableException("Unable to make blackbox request: Fetching ticket for blackbox failed")
        try:
            response = requests.get(self.blackbox.url,
                                    params=dict(method=method, **params),
                                    headers={'X-Ya-Service-Ticket': ticket_for_blackbox},
                                    timeout=BLACKBOX_TIMEOUT_SEC)
            if response.status_code in (502, 503, 504):
                current_app.logger.error(
                    "Unable to call blackbox method {}. Response code is {}".format(method, response.status_code),
                    extra=dict(response=response.content))
                raise BlackboxRetryableException("Blackbox responded not 200 OK ({})". format(response.status_code))
            elif response.status_code != 200:
                current_app.logger.error(
                    "Unable to call blackbox method {}. Response code is {}".format(method, response.status_code),
                    extra=dict(response=response.content))
                raise BlackboxFatalException("Blackbox responded not 200 OK ({})".format(response.status_code))

            response_json = response.json()

            if 'error' in response_json and response_json['error'] != 'OK':
                current_app.logger.error("Blackbox responded with error on method {}".format(method), extra=dict(response=response_json))
                raise BlackboxFatalException("Blackbox returned error details")

            return response_json
        except TimeoutError:
            current_app.logger.exception("Call Blackbox method {} fails with timeout after {} sec".format(method, BLACKBOX_TIMEOUT_SEC))
            raise BlackboxRetryableException("Call Blackbox method {} fails with timeout after {} sec".format(method, BLACKBOX_TIMEOUT_SEC))

        except Exception:
            current_app.logger.exception("Unable to call method {} for unknown reason (exception raised)".format(method))
            raise BlackboxFatalException("Unable to call method {} for unknown reason (exception raised)".format(method))
