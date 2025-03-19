import logging
from enum import Enum

from bot_utils.errors import LogicalError
from clients.staff import StaffUser
from database.crud import create_query, delete_query, read_query, update_query


class PersonContactType(Enum):
    STAFF_LOGIN = 1
    STAFF_LINK = 2
    TELEGRAM_LINK = 3


class User(object):
    __slots__ = (
        "chat_id",
        "login_tg",
        "name_staff",
        "bot_admin",
        "cloud",
        "send_reminders",
        "incidents",
        "ask_feedback",
        "login_staff",
        "allowed",
    )

    def __init__(self, chat_id=None, login_tg=None, login_staff=None):
        self.login_staff = login_staff
        self.login_tg = login_tg
        staff_data = self.get_staff_data()
        if not any([login_staff, login_tg, staff_data]):
            raise LogicalError("No user init-data was provided")

        if not self.login_staff:
            self.login_staff = staff_data.staff_login

        if not self.login_tg:
            self.login_tg = staff_data.telegram_login

        self.chat_id = chat_id
        self.name_staff = staff_data.fullname
        self.bot_admin = False
        self.allowed = not staff_data.is_fired
        self.cloud = staff_data.is_cloud
        self.send_reminders = True
        self.incidents = False
        self.ask_feedback = True

        """
            Check database, if user exists -> load from database
            Else basic initialisation with default values
        """
        _user = self.__get_user_from_database()
        if _user:
            self.chat_id = _user[0]
            self.name_staff = _user[2]
            self.bot_admin = _user[3]
            self.allowed = not staff_data.is_fired
            self.cloud = _user[4]
            self.send_reminders = _user[5]
            self.incidents = _user[6]
            self.ask_feedback = _user[7]
            return

    def __dict__(self):
        return {_var: self.__getattribute__(_var) for _var in self.__slots__}

    def __get_user_from_database(self):
        if self.login_staff:
            condition = f"WHERE login_staff='{self.login_staff}'"
        elif self.login_tg:
            condition = f"WHERE login_tg='{self.login_tg}'"
        elif self.chat_id:
            condition = f"WHERE chat_id='{self.chat_id}'"
        else:
            return

        try:
            return read_query(table="users", condition=condition, fields=list(self.__slots__))[0]
        except IndexError:
            logging.info(f"user {self.login_tg} not exists in database")

    def __delete_user_from_database(self):
        return delete_query(table="users", condition=f"WHERE login_staff='{self.login_staff}'")

    def __update_user_fields_in_database(self):
        raise NotImplementedError()

    def add_user(self):
        return create_query(table="users", data=self.__dict__())

    def del_user(self):
        return delete_query(table="users", condition=f"WHERE login_staff='{self.login_staff}'")

    def set_ask_feedback(self, ask_feedback: bool):
        return update_query(
            table="users", condition=f"WHERE login_staff='{self.login_staff}'", fields={"ask_feedback": ask_feedback}
        )

    def update_user(self, **kwargs):
        raise NotImplementedError()

    def get_staff_data(self):
        return StaffUser(staff_login=self.login_staff, telegram_login=self.login_tg)

    @property
    def exists_in_db(self):
        return bool(self.__get_user_from_database())

    @classmethod
    def list_users(cls, **kwargs):
        raise NotImplementedError()

    @classmethod
    def users_count(cls):
        raise NotImplementedError()
