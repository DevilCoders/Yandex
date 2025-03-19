from typing import List

from bot_utils.users import User, PersonContactType
from clients.resps import ScheduleItem
from templaters.abstract import AbstractTemplater
from templaters.renderer import LinesRenderer, NewIncidentLine, IncidentSummaryLine, IssueHtmlWithLinkLine, \
    IssueHtmlWithPriorityLine, IssueHtmlWithComponentsLine, ResponsiblePersonHtmlWithContactsLine, \
    ResponsiblePersonHtmlWithLoginLine


class YandexMessengerTemplater(AbstractTemplater):
    def generate_single_string_output(self, team_name: str, primary: User, backup: User) -> List[str]:
        lines = [
            f"**{team_name.upper()}**:",
            ("@" + primary.login_staff) if primary.login_staff else "__нет дежурного__",
        ]
        if primary.login_staff != backup.login_staff and backup.login_staff:
            lines.append(", @" + backup.login_staff)

        return lines

    def generate_full_team_output(
        self, name, primary: User, backup: User, teamlead: User, chat_link: str, ticket: str, call_backup: bool
    ) -> List[str]:
        # If schedule is not specified, teamlead will be set as primary!
        lines = [f"prim > @{primary.login_staff}"]
        if primary.login_staff != backup.login_staff:
            lines.append(f"back > {'@' if call_backup else ''}{backup.login_staff}")

        lines.append(f"lead > [{teamlead.login_staff}](https://staff.yandex-team.ru/{teamlead.login_staff})")
        if chat_link:
            lines.append(f"telegram chat > [join]({chat_link})")
        if ticket:
            lines.append(f"ticket > [{ticket}](https://st.yandex-team.ru/{ticket})")

        return lines

    def generate_user_duties_output(self, schedule: List[ScheduleItem]):
        if not schedule:
            return ["Не нашел твоих дежурств на ближайший месяц"]

        messages = ["*Твои ближайшие дежурства*", ""]
        for shift in schedule:
            date_range = self.render_russian_date_range(shift.start, shift.end)
            messages.append(f"{date_range}: *{shift.role}*-дежурный в команде `{shift.team_name}`\n")
        return messages

    def generate_incident_output(
        self, issue_key: str, summary: str,
        assignee: User, priority: str, components: List[str]
    ) -> List[str]:
        return LinesRenderer([
            NewIncidentLine(),
            IncidentSummaryLine(summary=summary),
            ResponsiblePersonHtmlWithLoginLine(assignee=assignee),
            IssueHtmlWithLinkLine(issue_key=issue_key),
            IssueHtmlWithPriorityLine(priority=priority),
            IssueHtmlWithComponentsLine(components=components),
            ResponsiblePersonHtmlWithContactsLine(
                assignee=assignee,
                supported_contacts=[
                    PersonContactType.STAFF_LOGIN,
                    PersonContactType.STAFF_LINK,
                    PersonContactType.TELEGRAM_LINK
                ]
            ),
        ]).render()

    def generate_duty_reminder(self, role: str, second_duty_login: str, team_name: str):
        output = [
            f"Завтра ты {role}-дежурный в команде {team_name}",
            f"Вместе с тобой будет дежурить @{second_duty_login}",
        ]
        return output

    def render_login_tg(self, user: User, call=False):
        # TODO (andgein): this method should be removed
        if user.login_tg:
            return f"(https://t.me/{user.login_tg})"
        return ""

    def render_tg_staff_logins(self, user: User, call=False):
        return f"https://staff.yandex-team.ru/{user.login_staff}"

    def bold(self, text: str):
        return "**" + text + "**"

    def mono(self, text: str) -> str:
        return "`" + text + "`"
