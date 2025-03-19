#!/usr/bin/env python3
'''Auth in icecream'''

import logging
import socket
import flask
from blackboxer import Blackbox
from lib import utils, Mongo


def auth():
    '''check auth'''
    log = logging.getLogger()
    if flask.request.path == '/v1/ping':
        return

    if not getattr(auth, "mongocli", None):
        log.debug("Initialize mongo client")
        auth.mongocli = Mongo()

    if not getattr(auth, "blackbox", None):
        cfg = utils.load_config()
        auth.blackbox = Blackbox.from_url(url=cfg.blackbox)
    if not auth.blackbox:
        log.error("Failed to initialize blackbox client")
        flask.abort(401, "Blackbox client initialization failed")

    ip_addr = socket.gethostbyaddr(socket.gethostname())[2][0]
    payload = {
        "format": "json",
        "host": "yab.yandex-team.ru",
        "userip": ip_addr,
    }
    method = "oauth"
    if flask.request.headers.get('Authorization'):
        oauth_token = flask.request.headers.get('Authorization').split(' ')[1]
        payload["oauth_token"] = oauth_token
        # XXX: do not log oauth token
        # log.debug("oauth token: %s", oauth_token)
    else:
        method = "sessionid"
        payload["sessionid"] = flask.request.cookies.get('Session_id')
        payload["sessionid2"] = flask.request.cookies.get('sessionid2')

    log.info("Send request to blackbox with payload %s", payload)
    try:
        data = getattr(auth.blackbox, method)(**payload)
        log.debug("Blackbox response: %s", data)
        login = data['login']
        log.info("Request to blackbox success, user login is: %s", login)
    except Exception as exc:  # pylint: disable=broad-except
        log.exception("Request to blackbox failed")
        flask.abort(401, "Blackbox request fail, not authorized {}".format(exc))


    try:
        users = auth.mongocli.users()
        log.debug("User list from mongodb: %s", users)
    except Exception as error:  # pylint: disable=broad-except
        log.debug("mongodb failed to list users: %s", error)
        flask.abort(401, "Mongodb failed to list users, not authorized")

    if login not in users:
        log.info("Login failed. User %s not in acl", login)
        flask.abort(401, "User not found")

    log.info("Login under ther user %s - success", login)


if __name__ == '__main__':
    auth()
