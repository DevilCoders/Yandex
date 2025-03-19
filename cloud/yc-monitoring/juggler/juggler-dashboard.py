#!/usr/bin/env python3
# Script for managing Juggler dashboards.
# Supports Jinja.
import argparse
import logging
import os.path
import pwd
import re
from datetime import datetime

import requests
import yaml

from jinja2 import FileSystemLoader, Environment

JUGGLER_BASE_URL = "https://juggler.yandex-team.ru/dashboards/"
JUGGLER_API_BASE_URL = "http://juggler-api.search.yandex.net/v2/dashboards/"
OAUTH_ENV_VAR = "JUGGLER_OAUTH_TOKEN"
OAUTH_FILE = ".juggler.oauth"
OAUTH_URL = "https://oauth.yandex-team.ru/authorize?response_type=token&client_id=cd178dcdc31a4ed79f42467f2d89b0d0"
REPO = "[yc-monitoring](https://bb.yandex-team.ru/projects/CLOUD/repos/yc-monitoring/browse/juggler)"


class Mode:
    DOWNLOAD = "download"
    UPLOAD = "upload"
    ALL = [DOWNLOAD, UPLOAD]


class JugglerClient:
    def __init__(self):
        self.headers = {
            "Authorization": "OAuth " + get_oauth_token(),
        }

    def get_dashboards(self, address: str):
        response = requests.post(JUGGLER_API_BASE_URL + "get_dashboards", headers=self.headers, json={
            "filters": [{"address": address}]
        })
        return response.json()["items"]

    def set_dashboards(self, dashboard: dict):
        response = requests.post(JUGGLER_API_BASE_URL + "set_dashboard", headers=self.headers, json=dashboard)
        if response.status_code != 200:
            raise Exception(response.text)


def get_oauth_token():
    oauth_token = os.getenv(OAUTH_ENV_VAR)
    if oauth_token:
        return oauth_token
    try:
        with open(OAUTH_FILE) as f:
            return f.read().strip()
    except FileNotFoundError:
        raise Exception("OAuth token not found! Checked: {!r} environment variable, {!r} file.\n"
                        "Issue new token: {}".format(OAUTH_ENV_VAR, OAUTH_FILE, OAUTH_URL))


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=Mode.ALL)
    parser.add_argument("--file", help="File name(s) to upload or download", nargs="+")
    parser.add_argument("--address", help="Dashboard address (for download), for example `ycloud_vpc_api` or `ycloud_virtual_network`")
    parser.add_argument("--apply", action="store_true", help="Apply changes (for upload)")
    parser.add_argument("--debug", action="store_true", help="Be more verbose")
    parser.add_argument("--test", action="store_true", help="Upload dashboard to {{address}}_test")
    return parser.parse_args()


def require_argument(args, name: str, description: str):
    if not args.__getattribute__(name):
        raise Exception("--{} ({}) is required, but not specified.".format(name, description))


def download(args):
    require_argument(args, "address", "part of the dashboard url in juggler, i.e. ycloud_virtual_network")
    require_argument(args, "file", "file to save dashboard")
    client = JugglerClient()

    dashboards = client.get_dashboards(args.address)
    if len(dashboards) == 0:
        logging.error("Dashboard with address %r not found.", args.address)
        return

    if len(dashboards) > 1:
        logging.debug(str(dashboards))
        raise Exception("Found {} dashboard(s), expected strictly 1.".format(len(dashboards)))

    with open(args.file[0], "w") as f:
        yaml.safe_dump(dashboards[0], f, allow_unicode=True)
    logging.info("Dashboard was saved to: %s", args.file)


def upload_file(file: str, test: bool, apply: bool):
    client = JugglerClient()

    logging.info("Reading dashboard file: %s", file)
    template_env = Environment(loader=FileSystemLoader(searchpath="./"), extensions=['jinja2.ext.do'])
    template = template_env.get_template(file)
    render_text = template.render()
    logging.debug("Render result:\n%s", render_text)

    dashboard_object = yaml.safe_load(render_text)

    address = re.sub(r"\..*", "", os.path.basename(file))
    if test:
        address += "_test"
        dashboard_object["name"] += " [TEST]"
    logging.info("Dashboard URL: %s", JUGGLER_BASE_URL + address)

    dashboards = client.get_dashboards(address)
    if len(dashboards) == 0:
        logging.info("Dashboard doesn't exist in Juggler (will be created)")
    elif len(dashboards) == 1:
        dashboard_id = dashboards[0]["dashboard_id"]
        logging.info("Dashboard exists in Juggler with ID: %s", dashboard_id)
        dashboard_object["dashboard_id"] = dashboard_id
    else:
        raise Exception("Found {} dashboard(s), expected 0 or 1.".format(len(dashboards)))
    dashboard_object["address"] = address
    if dashboard_object.get("description") is None:
        dashboard_object["description"] = "Авто-обновлено: {} ({}) из {}.".format(
            pwd.getpwuid(os.getuid()).pw_name, datetime.now().strftime("%Y-%m-%d %H:%M:%S"), REPO)

    if apply:
        client.set_dashboards(dashboard_object)
        logging.info("Dashboard has been updated successfully in Juggler.")
    else:
        logging.info("Dry-run, not updating Juggler.")


def upload(args):
    require_argument(args, "file", "file list with jinja templates for uploading")

    for idx, file in enumerate(args.file, 1):
        logging.info("File %d of %d...", idx, len(args.file))
        upload_file(file, args.test, args.apply)
        logging.info("")


def main():
    args = parse_args()
    logging.basicConfig(format="%(asctime)s %(message)s", level=logging.DEBUG if args.debug else logging.INFO)

    if args.mode == Mode.DOWNLOAD:
        download(args)
    elif args.mode == Mode.UPLOAD:
        upload(args)
    else:
        raise Exception("Unsupported mode.")


if __name__ == "__main__":
    main()
