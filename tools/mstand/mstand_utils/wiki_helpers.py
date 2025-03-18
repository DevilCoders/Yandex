import json

import yaqutils.requests_helpers as urequests

BASE_URL = "https://wiki.yandex-team.ru/"
API_URL = "https://wiki-api.yandex-team.ru/_api/frontend/"


def create_page(page, title, body, wiki_token):
    url = API_URL + page
    data = {
        "title": title,
        "body": body,
    }
    headers = {
        "Authorization": "OAuth " + wiki_token,
        "Content-Type": "application/json",
    }

    urequests.strict_request("POST", url=url, data=json.dumps(data), headers=headers)


def upload_results(title, body, page_name, wiki_token):
    page = 'users/robot-mstand/results/' + page_name
    create_page(page, title, body, wiki_token=wiki_token)
    return BASE_URL + page
