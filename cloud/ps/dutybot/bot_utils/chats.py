from typing import Literal

from database.crud import read_query, create_query


class Chat:
    __slots__ = (
        "id",
        "name",
        "component",
        "admin",
        "incidents",
        "allowed",
        "duty_noargs",
        "default_duty_team",
        "messenger",
    )

    def __init__(self, messenger: Literal["telegram", "yandex", "slack"], chat_id, chat_name=None):
        if messenger not in ("telegram", "yandex", "slack"):
            raise ValueError(f"Unknown messenger: {messenger}")

        self.id = chat_id
        self.name = chat_name
        self.component = None
        self.admin = None
        self.incidents = False
        self.allowed = False
        self.duty_noargs = False
        self.default_duty_team = None
        self.messenger = messenger

        _chat = self.__get_chat_from_database()

        # Order is like in slots
        if _chat:
            self.name = _chat[1]
            if _chat[2]:
                self.component = _chat[2].split(",")
            self.admin = _chat[3]
            self.incidents = _chat[4]
            self.allowed = True
            self.duty_noargs = _chat[6]
            self.default_duty_team = _chat[7]
            self.messenger = _chat[8]

    def __get_chat_from_database(self):
        try:
            return read_query(
                table="chats", condition=f"WHERE chat_id = '{self.id}' AND messenger = '{self.messenger}'"
            )[0]
        except IndexError:
            return None

    def __delete_chat_from_database(self):
        raise NotImplementedError()

    def add_chat(self):
        fields = {"chat_id": self.id, "chat_name": self.name, "messenger": self.messenger}
        return create_query(table="chats", data=fields)

    @classmethod
    def get_all_inc_notify_chats(cls):
        return read_query(
            table="chats",
            condition="WHERE incidents = True and messenger = 'telegram'",
            fields=["chat_id", "component"],
        )
