import logging

from bot_utils.config import Config
from bot_utils.request import Requester


class StaffClient:
    def __init__(self):
        self.endpoint = Config.staff_endpoint
        self.headers = {"Authorization": f"OAuth {Config.ya_staff}"}
        self.user_url = f"{self.endpoint}/persons?"
        self.fields = [
            "login",
            "official.is_dismissed",
            "department_group.parent.url",
            "accounts",
            "name",
            "department_group.department.name",
        ]
        self.params = {"_fields": ",".join(self.fields)}

    def get_user_data_from_staff(self, staff_login=None, telegram_login=None):
        if staff_login and telegram_login:
            logging.error("staff login and telegram login was provided simultaneously, using staff login")
            telegram_login = None

        if staff_login is not None:
            self.params["login"] = staff_login
        elif telegram_login is not None:
            self.params["accounts.value_lower"] = telegram_login.lower()
        else:
            logging.error("staff login and telegram login are both None, can't get such user")
            return {}

        req = Requester().request(url=self.user_url, headers=self.headers, params=self.params)
        if req:
            res = {}
            for person in req["result"]:
                res = person
                if not person.get("official", {}).get("is_dismissed", False):
                    return person
            return res
        return {}


class StaffUser:
    def __init__(self, staff_login=None, telegram_login=None):
        self.client = StaffClient()
        self.staff_login = staff_login
        self.cloud_department = Config.cloud_department
        self.telegram_login = telegram_login
        if self.telegram_login:
            self.raw_info = self.client.get_user_data_from_staff(telegram_login=self.telegram_login)
            self.staff_login = self.raw_info.get("login")
        else:
            self.raw_info = self.client.get_user_data_from_staff(staff_login=self.staff_login)
            self.telegram_login = self._get_telegram_login_by_staff()
        self.is_fired = self._is_user_fired()
        self.is_cloud = self._is_user_from_cloud_department()
        self.fullname = self._generate_fullname()
        self.department = self._get_user_department()

    def _get_telegram_login_by_staff(self):
        accounts = self.raw_info.get("accounts", [])
        # Find public telegram accounts first
        for account in accounts:
            if account.get("type") == "telegram" and not account.get("private"):
                return account.get("value_lower")

        # If no public accounts was found get private
        for account in accounts:
            if account.get("type") == "telegram" and account.get("private"):
                return account.get("value_lower")
        return

    def _is_user_fired(self):
        return self.raw_info.get("official", {}).get("is_dismissed", True)

    def _is_user_from_cloud_department(self):
        return (
            self.raw_info.get("department_group", {}).get("parent", {}).get("url", "").startswith(self.cloud_department)
        )

    def _generate_fullname(self):
        first_name = f"{self.raw_info.get('name', {}).get('first', {}).get('ru')}"
        second_name = f"{self.raw_info.get('name', {}).get('last', {}).get('ru')}"
        return f"{first_name} {second_name}"

    def _get_user_department(self):
        return self.raw_info.get("department_group", {}).get("department", {}).get("name", {}).get("full", {}).get("ru")
