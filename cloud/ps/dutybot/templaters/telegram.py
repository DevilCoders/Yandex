import logging
from typing import List

from telegram.utils.helpers import escape_markdown as md

from bot_utils.users import User, PersonContactType
from clients.resps import ScheduleItem
from templaters.abstract import AbstractTemplater
from templaters.renderer import LinesRenderer, NewIncidentLine, IncidentSummaryLine, IssueHtmlWithLinkLine, \
    IssueHtmlWithPriorityLine, IssueHtmlWithComponentsLine, ResponsiblePersonHtmlWithContactsLine, \
    ResponsiblePersonHtmlWithLinkAndNameLine


class TelegramTemplater(AbstractTemplater):
    def generate_single_string_output(self, name, primary, backup):
        lines = []
        lines.append(f"*{name.upper()}*")
        lines.append(f"[{primary.login_staff}]{self.render_login_tg(primary)}")
        if primary.login_staff != backup.login_staff:
            lines.append(f"([{backup.login_staff}]{self.render_login_tg(backup)})")

        for user in {primary, backup}:
            if user.login_tg is None:
                lines.append(f"У {md(user.login_staff)} телеграм на стаффе не указан.")
                logging.warning(f"У {user.login_staff} телеграм на стаффе не указан.")

        return lines

    def generate_full_team_output(self, name, primary, backup, teamlead, chat_link, ticket, call_backup):
        # If schedule is not specified, teamlead will be set as primary!
        lines = []
        lines.append(f"prim > {self.render_login_tg(primary, True)}{md(primary.login_staff)}")
        if primary.login_staff != backup.login_staff:
            if call_backup:
                lines.append(f"back > {self.render_login_tg(backup, call_backup)}{md(backup.login_staff)}")
            else:
                lines.append(f"back > [{md(backup.login_staff)}]{self.render_login_tg(backup, call_backup)}")

        lines.append(f"lead > [{md(teamlead.login_staff)}]{self.render_login_tg(teamlead)}")
        if chat_link:
            lines.append(f"chat > [join]({chat_link})")
        if ticket:
            lines.append(f"ticket > [{ticket}](https://st.yandex-team.ru/{ticket})")

        for user in {primary, backup, teamlead}:
            if user.login_tg is None:
                lines.append(f"У {md(user.login_staff)} телеграм на стаффе не указан.")
                logging.warning(f"У {user.login_staff} телеграм на стаффе не указан.")

        return lines

    def generate_user_duties_output(self, schedule: List[ScheduleItem]):
        if not schedule:
            return ["Не нашел твоих дежурств на ближайший месяц"]

        messages = ["*Твои ближайшие дежурства*", ""]
        for shift in schedule:
            date_range = self.render_russian_date_range(shift.start, shift.end)
            messages.append(f"{date_range}: *{shift.role}*-дежурный в команде `{shift.team_name}`\n")
        return messages

    def generate_incident_output(self, issue_key: str, summary: str, assignee: User, priority: str, components: List[str]):
        return LinesRenderer([
            NewIncidentLine(),
            IncidentSummaryLine(summary=summary),
            ResponsiblePersonHtmlWithLinkAndNameLine(assignee=assignee),
            IssueHtmlWithLinkLine(issue_key=issue_key),
            IssueHtmlWithPriorityLine(priority=priority),
            IssueHtmlWithComponentsLine(components=components),
            ResponsiblePersonHtmlWithContactsLine(
                assignee=assignee,
                supported_contacts=[
                    PersonContactType.STAFF_LINK,
                    PersonContactType.TELEGRAM_LINK,
                ]
            ),
        ]).render()

    def generate_duty_reminder(self, role, second_duty_login, team):
        output = [f"Завтра ты {role}-дежурный в команде {team}", f"Вместе с тобой будет дежурить @{second_duty_login}"]
        return output

    def render_login_tg(self, user, call=False):
        if user.login_tg is not None:
            if call:
                return f"@{md(user.login_tg)} "
            return f"(https://t.me/{user.login_tg})"
        return ""

    def render_tg_staff_logins(self, user, call=False):
        return f"{self.render_login_tg(user, call)}{md(user.login_staff)}"

    def bold(self, text: str) -> str:
        return "*" + text + "*"

    def mono(self, text: str) -> str:
        return "`" + text + "`"
