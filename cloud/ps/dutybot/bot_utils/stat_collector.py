import logging
from datetime import datetime

from database.crud import create_query


class DutyStats:
    __slots__ = (
        "caller",
        "chat",
        "message_text",
        "called_team",
        "called_team_primary",
        "called_team_backup",
        "call_date",
    )

    def __init__(self, caller, chat, message, team, primary, backup):
        self.caller = caller
        self.chat = chat
        self.message_text = message
        self.called_team = team
        self.called_team_primary = primary
        self.called_team_backup = backup
        self.call_date = datetime.now()

    def __dict__(self):
        return {var: getattr(self, var) for var in self.__slots__}

    def write_record(self):
        try:
            return create_query(table="bot_stats", data=self.__dict__())
        except Exception as err:
            logging.error(f"cant write stat record to database due to {err}")
            return False
