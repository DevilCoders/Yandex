import string

try:
    from http.cookies import BaseCookie
except ImportError:
    from Cookie import BaseCookie

import requests


_valid_chars = string.ascii_letters + string.digits + "!#$%&'*+-.^_`|~"


ANTIADB_ROBOT_LOGIN = 'robot-antiadb'

ABC_ANTIADBLOCK_ID = 1526
ABC_MAIN_SUPPORT_ROLE_ID = "antiadb_main"
ABC_RESERVE_SUPPORT_ROLE_ID = "antiadb_reserve"
ABC_ANTIADBLOCK_SUPPORT = "https://abc-back.yandex-team.ru/api/v4/services/{id}/on_duty/".format(id=ABC_ANTIADBLOCK_ID)


def parse_and_validate_cookies(cookies):
    if not cookies:
        return []
    cookie_list = []
    base_cookie = BaseCookie()
    for chunk in cookies.strip(';').split(';'):
        if '=' in chunk:
            key, val = chunk.split('=', 1)
            if any(c not in _valid_chars for c in key):
                raise KeyError("Cookie key error")
            base_cookie.load(chunk)
        else:
            raise Exception(" '=' not in cookie")
    for key, val in base_cookie.items():
        cookie_dict = dict()
        cookie_dict["name"] = key
        cookie_dict["value"] = val.value
        if val.get("path"):
            cookie_dict["path"] = val.get("path")
        if val.get("secure"):
            cookie_dict["secure"] = True
        cookie_list.append(cookie_dict)

    return cookie_list


def get_abc_duty(token):
    # Забираем дежурных из ABC
    service_duty = requests.get(
        ABC_ANTIADBLOCK_SUPPORT,
        headers={'Authorization': 'OAuth {}'.format(token)}).json()

    duty_roles = {element['schedule']['slug']: element['person']['login'] for element in service_duty}

    main = duty_roles.get(ABC_MAIN_SUPPORT_ROLE_ID, ANTIADB_ROBOT_LOGIN)
    reserved = duty_roles.get(ABC_RESERVE_SUPPORT_ROLE_ID, ANTIADB_ROBOT_LOGIN)

    return main, reserved
