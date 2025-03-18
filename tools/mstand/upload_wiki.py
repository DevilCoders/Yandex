#!/usr/bin/env python2.7
# -*- coding: utf-8 -*-

import argparse
import logging
import sys

import requests

import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc


def upload_wiki(api, page, title, body, token):
    url = api + page
    data = {
        "title": title,
        "body": body,
    }
    headers = {
        "Authorization": "OAuth {}".format(token),
        "Content-Type": "application/json",
    }
    logging.info("url: %s", url)
    logging.info("data: %s", data)
    logging.info("headers: %s", data)
    response = requests.post(url=url, data=data, headers=headers)
    if response.status_code < 200 or response.status_code >= 300:
        raise Exception("Error with code {}: {}".format(response.status_code, response.text))


def parse_args():
    parser = argparse.ArgumentParser(description="Upload wiki")
    uargs.add_verbosity(parser)
    parser.add_argument(
        "--page",
        required=True,
        help="page path",
    )
    parser.add_argument(
        "--title",
        required=True,
        help="page title",
    )
    parser.add_argument(
        "--body",
        help="page body (or file path)",
    )
    parser.add_argument(
        "--token",
        required=True,
        help="token to access wiki",
    )
    parser.add_argument(
        "--body-from-file",
        action="store_true",
        help="read body from file",
    )
    parser.add_argument(
        "--api",
        default="https://wiki-api.yandex-team.ru/_api/frontend/",
        help="wiki API url",
    )
    return parser.parse_args()


def main():
    args = parse_args()
    umisc.configure_logger(args.verbose, args.quiet)
    if args.body_from_file:
        if args.body:
            with open(args.body) as f:
                args.body = f.read()
        else:
            args.body = sys.stdin.read()
    upload_wiki(args.api, args.page, args.title, args.body, args.token)


if __name__ == "__main__":
    main()
