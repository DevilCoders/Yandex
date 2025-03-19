import logging
import datetime
from typing import Set

from bot_utils.errors import LogicalError
from bot_utils.users import User
from clients.resps import RespsService
from database.crud import read_query
from templaters.abstract import AbstractTemplater


class Team:
    def __init__(self, team: str, timestamp: datetime.datetime, templater: AbstractTemplater):
        self.name = team

        resps_response = RespsService(self.name, timestamp)

        # Expect that every team have teamlead field filled in resps
        self.teamlead = User(login_staff=resps_response.teamlead)
        self.ticket = resps_response.duty_ticket

        # TODO: move all fields below to resps client, atm in DB
        self.alias_list = []
        self.startrack_component, self.chat_link, self.call_backup, self.ask_feedback, self.send_reminders = (
            None,
            None,
            True,
            True,
            True,
        )

        optional_params = self._get_optional_params()
        if optional_params is not None:
            self.startrack_component = optional_params[0]
            self.alias_list = optional_params[3].split(",")
            self.chat_link = optional_params[1]
            self.call_backup = bool(optional_params[2])
            self.ask_feedback = bool(optional_params[4])
            self.send_reminders = bool(optional_params[5])

        # If oncall was not provided by timetable -> show teamlead as both
        self.primary, self.backup = self.teamlead, self.teamlead
        self.timetable_filled = False

        oncall_data = resps_response.actual_oncall
        if oncall_data:
            if "primary" in oncall_data:
                try:
                    self.primary = User(login_staff=oncall_data.get("primary"))
                    self.timetable_filled = True
                except LogicalError:
                    logging.warning(f"Timetable is not filled in for {self.name}, setting teamlead")
            else:
                logging.warning(f"Timetable is not filled in for {self.name}, setting teamlead")

            # In some teams we have only one oncall specified
            if "backup" in oncall_data:
                try:
                    self.backup = User(login_staff=oncall_data.get("backup"))
                except Exception:
                    self.backup = self.primary
            else:
                self.backup = self.primary

        self.single_string_output = self._get_single_string_output(templater)
        self.classic_output = self._get_classic_output(templater)

    def _get_optional_params(self):
        component = read_query(
            "components",
            f"WHERE component_name = '{self.name}'",
            ["map_components", "chat_link", "call_backup", "custom_dict", "ask_feedback", "send_reminders"],
        )
        try:
            return component[0]
        except IndexError:
            return None

    def _get_single_string_output(self, templater: AbstractTemplater):
        try:
            return templater.generate_single_string_output(self.name, primary=self.primary, backup=self.backup)
        except Exception as e:
            logging.exception(f"Can not generate single string output for team {self.name}", exc_info=e)
            return ""

    def _get_classic_output(self, templater: AbstractTemplater):
        try:
            return templater.generate_full_team_output(
                name=self.name,
                primary=self.primary,
                backup=self.backup,
                teamlead=self.teamlead,
                chat_link=self.chat_link,
                ticket=self.ticket,
                call_backup=self.call_backup,
            )
        except:
            return ""

    def get_duty_telegram_usernames(self) -> Set[str]:
        duty = set()
        if self.primary.login_tg is not None:
            duty.add(self.primary.login_tg)

        if self.backup.login_tg is not None:
            duty.add(self.backup.login_tg)

        return duty


class FakeTeam:
    """Class for storing teams which don't exist in the database"""

    def __init__(self, team_name: str):
        self.name = team_name
