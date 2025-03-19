"""Test routes in admin section"""

from common import bootstrap_api_req

from conftest import DB_VERSION


def test_get_version(bootstrap_api):
    bootstrap_api_req("get", "admin/version", expected_json={
        "code": 200,
        "data": {
            "version": DB_VERSION,
        },
        "error_message": None,
        "status": "success",
    })

    bootstrap_api_req("post", "admin/version", expected_json={
        "data": None,
        "code": 405,
        "status": "fail",
        "error_message": "MethodNotAllowed: 405 Method Not Allowed: The method is not allowed for the requested URL."
    })


def test_invalid_path(bootstrap_api):
    bootstrap_api_req("get", "unexisting/path", expected_json={
        "data": None,
        "code": 404,
        "status": "fail",
        "error_message": ("NotFound: 404 Not Found: The requested URL was not found on the server. If you entered the "
                          "URL manually please check your spelling and try again."),
    })
