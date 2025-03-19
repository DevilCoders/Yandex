import requests
import json
import logging
import urllib3

from app.utils.config import Config

urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)

class UserNotExists(Exception):
    """Custom staff exception that raises when user was
    not found on staff
    """

def get_data_from_staff(telegram=None, staff=None):
    token = Config.ya_staff
    endpoint = Config.staff_endpoint
    department = Config.cloud_department
    headers = {'Authorization': 'OAuth {}'.format(token)}
    if telegram == None:
        params =  {'login': staff, '_one': '1'}
    else:
        params =  {'accounts.value_lower': telegram.lower(), '_one': '1'}
    with requests.Session() as staff:
        request = staff.get(f"{endpoint}/persons?",
                            headers=headers,
                            params=params)
        data = request.json()
        try:
            is_fired = data['official']['is_dismissed']
            name_staff = f"{data['name']['first']['ru']} {data['name']['last']['ru']}"
            cloud = data['department_group']['parent']['url'].startswith(department)
            login_staff = data['login']
            if is_fired is True:
                raise(UserNotExists)
            if telegram == None:
                telegram = [account.get("value_lower") for account in data.get('accounts') if account.get('type') == 'telegram']
                if telegram:
                    telegram = telegram[0]
                return {'login_tg': telegram,
                        'name_staff': name_staff,
                        'login_staff': login_staff,
                        'allowed': True,
                        'cloud': cloud,
                        'duty_notify': True}
            return {'name_staff': name_staff,
                    'login_staff': login_staff,
                    'allowed': True,
                    'cloud': cloud,
                    'duty_notify': True}
        except (UserNotExists, KeyError):
            # If user wasn't found on staff just return blank data and ignore him
            logging.info(f"[staff.get_data_from_staff] User is not yandex employee or doesn't have telegram on staff")
            return {'name_staff': 'None',
                    'login_staff': 'None',
                    'allowed': 'False',
                    'cloud': 'False',
                    'duty_notify': 'False'}






