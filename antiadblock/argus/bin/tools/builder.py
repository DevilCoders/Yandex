import json
import os

import requests

from sandbox.projects.antiadblock.aab_automerge_ext import (
    AdblockChromeExtension,
    AdblockFirefoxExtension,
    AdblockplusChromeExtension,
    AdblockplusFirefoxExtension,
    AdguardChromeExtension,
    AdguardFirefoxExtension,
    UblockChromeExtension,
    UblockFirefoxExtension,
)
from sandbox.projects.antiadblock.aab_argus_utils import (
    SELENOID_CONFIG,
    AntiadblockChromeExtensionModCookie,
    AntiadblockChromeExtensionModHeader,
    AntiadblockFirefoxExtensionModCookie,
    AntiadblockFirefoxExtensionModHeader,
    get_extension_http_url,
)


def download_extension(modheader):
    modheader_url = get_extension_http_url(modheader, sync_resource=False)
    print(modheader_url)
    response = requests.get(modheader_url)
    response.raise_for_status()

    extension_filename = modheader_url.split('/')[-1]
    if not os.path.exists(extension_filename):
        with open(extension_filename, 'wb') as f:
            for chunk in response:
                f.write(chunk)
    return "./tools/" + extension_filename


def create_extensions_json():
    results = dict(
        adblocks=dict(
            ADBLOCK_CHROME=get_extension_http_url(AdblockChromeExtension, sync_resource=False),
            ADBLOCK_FIREFOX=get_extension_http_url(AdblockFirefoxExtension, sync_resource=False),
            ADBLOCK_PLUS_CHROME=get_extension_http_url(AdblockplusChromeExtension, sync_resource=False),
            ADBLOCK_PLUS_FIREFOX=get_extension_http_url(AdblockplusFirefoxExtension, sync_resource=False),
            ADGUARD_CHROME=get_extension_http_url(AdguardChromeExtension, sync_resource=False),
            ADGUARD_FIREFOX=get_extension_http_url(AdguardFirefoxExtension, sync_resource=False),
            UBLOCK_ORIGIN_CHROME=get_extension_http_url(UblockChromeExtension, sync_resource=False),
            UBLOCK_ORIGIN_FIREFOX=get_extension_http_url(UblockFirefoxExtension, sync_resource=False),
        ),
        modheaders=dict(
            CHROME_MODHEADER=download_extension(AntiadblockChromeExtensionModHeader),
            FIREFOX_MODHEADER=download_extension(AntiadblockFirefoxExtensionModHeader),
        ),
        modcookies=dict(
            CHROME_MODCOOKIE=download_extension(AntiadblockChromeExtensionModCookie),
            FIREFOX_MODCOOKIE=download_extension(AntiadblockFirefoxExtensionModCookie),
        ),
    )
    with open("extensions.json", "w") as fp:
        json.dump(results, fp, indent=4)


def create_profile_json():
    results = {
        "url_settings": [
            {
                "url": "https://www.kinopoisk.ru/",
                "selectors": [],
            },
        ],
        "cookies": "aabtesting=1",
        "proxy_cookies": "bltsr=1",
        "headers": {},
        "run_id": 2,
        "filters_list": [],
    }
    with open("profile.json", "w") as fp:
        json.dump(results, fp, indent=4)


def create_argus_settings_json():
    results = {
        "ARGUS_ENV": "NONE",
        "VIDEO": "DISABLE",
    }
    with open("settings.json", "w") as fp:
        json.dump(results, fp, indent=4)


def create_browsers_settings_json():
    with open("browsers.json", "w") as fp:
        json.dump(SELENOID_CONFIG, fp, indent=4)


def main():
    create_extensions_json()
    create_profile_json()
    create_argus_settings_json()
    create_browsers_settings_json()


if __name__ == "__main__":
    main()
