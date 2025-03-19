"""Main module"""

import argparse
import requests
import yaml

# FIXME: workaround for potential warnings with Yandex internal CA
from requests.packages.urllib3.exceptions import InsecureRequestWarning
requests.packages.urllib3.disable_warnings(InsecureRequestWarning)

_JUGGLER_HC_PATH = "v1/locks"
_BOOTSTRAP_API_URL = "https://api.bootstrap.cloud.yandex.net/"
_BOOTSTRAP_SECRETS_FILE = "~/.local/share/yc-bootstrap/bootstrap_secrets.yaml"


class EActions:
    JUGGLER_CHECK = "juggler-check"  # perform check for juggler
    ALL = [JUGGLER_CHECK]


def get_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Bootstrap api helper")

    subparsers = parser.add_subparsers(dest="action", required=True, help="Action to execute")

    # add arguments for juggler check
    juggler_check_parser = subparsers.add_parser(EActions.JUGGLER_CHECK)
    juggler_check_parser.add_argument(
        "-u", "--bootstrap-api-url", type=str, metavar="URL",
        default=_BOOTSTRAP_API_URL,
        help="Bootstrap api url ({} by default)".format(_BOOTSTRAP_API_URL)
    )
    juggler_check_parser.add_argument(
        "-s", "--bootstrap-secrets-file", type=str, metavar="FILE",
        default=_BOOTSTRAP_SECRETS_FILE,
        help="File with bootstrap secrets ({} by default)".format(_BOOTSTRAP_SECRETS_FILE)
    )

    return parser


def _get_bootstrap_api_token(secrets_file: str) -> str:
    with open(secrets_file) as f:
        return yaml.load(f, Loader=yaml.SafeLoader)["endpoints"]["bootstrap_api"]["token"]


def main_juggler_check(options: argparse.Namespace) -> int:
    """Perform juggler check, print result to stdout in nagios format"""
    try:
        url = "{}{}".format(options.bootstrap_api_url, _JUGGLER_HC_PATH)
        oauth_token = _get_bootstrap_api_token(options.bootstrap_secrets_file)
        headers = {
            "Authorization": "OAuth {}".format(oauth_token)
        }

        resp = requests.get(url, headers=headers, verify=False, timeout=3)

        if resp.status_code == 200:
            status_code = 0
            message = "Everything is ok!"
        else:
            status_code = 2
            message = "Check url <{}> returned status {}".format(url, resp.status_code)
    except Exception as e:
        status_code = 2
        message = "Check url <{}> failed with exception {}".format(url, str(e).replace("\n", " "))

    print("PASSIVE-CHECK:yc-bootstrap-api-hc;{};{}".format(status_code, message))

    return 0


_ACTIONS = {
    EActions.JUGGLER_CHECK: main_juggler_check,
}


def main(options: argparse.Namespace) -> int:
    return _ACTIONS[options.action](options)
