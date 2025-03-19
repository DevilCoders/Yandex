import datetime
from typing import List

from bot_utils.users import User
from clients.resps import ScheduleItem


class RussianDateVocabulary:
    months = [
        "январь",
        "февраль",
        "март",
        "апрель",
        "май",
        "июнь",
        "июль",
        "август",
        "сентябрь",
        "октябрь",
        "ноябрь",
        "декабрь",
    ]
    genitive_months = [
        "января",
        "февраля",
        "марта",
        "апреля",
        "мая",
        "июня",
        "июля",
        "августа",
        "сентября",
        "октября",
        "ноября",
        "декабря",
    ]


class AbstractTemplater:
    def generate_single_string_output(self, name, primary, backup):
        raise NotImplementedError()

    def generate_full_team_output(self, name, primary, backup, teamlead, chat_link, ticket, call_backup):
        raise NotImplementedError()

    def generate_user_duties_output(self, schedule: List[ScheduleItem]):
        raise NotImplementedError()

    def generate_incident_output(self, issue_key, summary, assignee, priority, components):
        raise NotImplementedError()

    def generate_duty_reminder(self, role, second_duty_login, team):
        raise NotImplementedError()

    def render_login_tg(self, user, call=False):
        raise NotImplementedError()

    def render_tg_staff_logins(self, user: User, call=False):
        raise NotImplementedError()

    def bold(self, text: str) -> str:
        raise NotImplementedError()

    def mono(self, text: str) -> str:
        raise NotImplementedError()

    @staticmethod
    def render_russian_date_range(start: datetime.date, end: datetime.date) -> str:
        if start > end:
            raise ValueError(f"Can not render date range from the future ({start}) to the past ({end})")

        if start == end:
            return f"{start.day} {RussianDateVocabulary.genitive_months[start.month - 1]}"

        if start.year == end.year and start.month == end.month:
            return f"С {start.day} по {end.day} {RussianDateVocabulary.genitive_months[start.month - 1]}"

        if start.year == end.year:
            return (
                f"С {start.day} {RussianDateVocabulary.genitive_months[start.month - 1]} "
                f"по {end.day} {RussianDateVocabulary.genitive_months[end.month - 1]}"
            )

        return (
            f"С {start.day} {RussianDateVocabulary.genitive_months[start.month - 1]} {start.year} "
            f"по {end.day} {RussianDateVocabulary.genitive_months[end.month - 1]} {end.year}"
        )
