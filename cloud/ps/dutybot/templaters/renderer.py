from typing import List, Optional

from bot_utils.users import User, PersonContactType


class BaseInfoLine:
    def __init__(self, is_multi_line: bool = False, **kwargs):
        self.data = kwargs
        self.is_multi_line = is_multi_line

    def render(self):
        raise NotImplementedError()


class NewIncidentLine(BaseInfoLine):
    def render(self):
        return "<b>Новый инцидент!</b>"


class IncidentSummaryLine(BaseInfoLine):
    def render(self):
        return self.data.get("summary", "")


class ResponsiblePersonStaffLoginLine(BaseInfoLine):
    def render(self):
        assignee: User = self.data.get("assignee")
        if assignee and assignee.login_staff:
            return f"@{assignee.login_staff}"

        return ""


class ResponsiblePersonStaffNameLine(BaseInfoLine):
    def render(self):
        assignee: User = self.data.get("assignee")
        if assignee and assignee.name_staff:
            return assignee.name_staff

        return ""


class ResponsiblePersonStaffLinkLine(BaseInfoLine):
    def render(self):
        assignee: User = self.data.get("assignee")
        if assignee and assignee.login_staff:
            return f"https://staff.yandex-team.ru/{assignee.login_staff}"

        return ""


class ResponsiblePersonTelegramLinkLine(BaseInfoLine):
    def render(self):
        assignee: User = self.data.get("assignee")
        if assignee and assignee.login_tg:
            return f"https://t.me/{assignee.login_tg}"

        return ""


class ResponsiblePersonHtmlWithLoginLine(BaseInfoLine):
    def render(self):
        assignee: User = self.data.get("assignee")
        staff_login_line = ResponsiblePersonStaffLoginLine(assignee=assignee).render() or 'неизвестен'

        return f"<b>Исполнитель:</b> {staff_login_line}"


class ResponsiblePersonHtmlWithLinkLine(BaseInfoLine):
    def render(self):
        assignee: User = self.data.get("assignee")
        staff_login_line = ResponsiblePersonStaffLoginLine(assignee=assignee).render()
        staff_link_line = ResponsiblePersonStaffLinkLine(assignee=assignee).render()
        if staff_login_line and staff_link_line:
            return f"<b>Исполнитель:</b> <a href='{staff_link_line}'>{staff_login_line}</a>"

        return ""


class ResponsiblePersonHtmlWithLinkAndNameLine(BaseInfoLine):
    def render(self):
        assignee: User = self.data.get("assignee")
        staff_name_line = ResponsiblePersonStaffNameLine(assignee=assignee).render()
        staff_link_line = ResponsiblePersonStaffLinkLine(assignee=assignee).render()
        if staff_name_line and staff_link_line:
            return f"<b>Исполнитель:</b> <a href='{staff_link_line}'>{staff_name_line}</a>"

        return ""


class IssueHtmlWithLinkLine(BaseInfoLine):
    def render(self):
        issue_key: str = self.data.get("issue_key", "")
        if issue_key:
            return f"<b>Тикет:</b> <a href='https://st.yandex-team.ru/{issue_key}'>{issue_key}</a>"

        return ""


class IssueHtmlWithPriorityLine(BaseInfoLine):
    def render(self):
        priority: str = self.data.get("priority", "")
        if priority:
            return f"<b>Приоритет:</b> {priority}"

        return ""


class IssueHtmlWithComponentsLine(BaseInfoLine):
    def render(self):
        components: List[str] = self.data.get("components", [])
        str_components_line = " ".join(components)
        if str_components_line:
            return f"<b>Компоненты:</b> {str_components_line}"

        return ""


class ResponsiblePersonHtmlWithContactsLine(BaseInfoLine):
    def __init__(self, supported_contacts: Optional[List[PersonContactType]] = None, **kwargs):
        """
        Explicitly mark this is a multi line, because the output is either an empty
        list, or a list with more than one element.
        """
        kwargs['is_multi_line'] = True
        super().__init__(**kwargs)
        self.supported_contacts = supported_contacts or []

    def render(self):
        assignee: User = self.data.get("assignee")
        executor_contact_entries = []
        for contact_type in self.supported_contacts:
            if contact_type == PersonContactType.STAFF_LOGIN:
                staff_login_line = ResponsiblePersonStaffLoginLine(assignee=assignee).render()
                executor_contact_entries.append(staff_login_line)

            elif contact_type == PersonContactType.STAFF_LINK:
                staff_link_line = ResponsiblePersonStaffLinkLine(assignee=assignee).render()
                executor_contact_entries.append(staff_link_line)

            elif contact_type == PersonContactType.TELEGRAM_LINK:
                telegram_link_line = ResponsiblePersonTelegramLinkLine(assignee=assignee).render()
                executor_contact_entries.append(telegram_link_line)

        if any(executor_contact_entries):
            executor_contact_entries.insert(0, '<b>Контакты исполнителя:</b>')

        return executor_contact_entries


class LinesRenderer:
    def __init__(self, info_lines: List[BaseInfoLine]):
        self.info_lines = info_lines

    def render(self):
        chunks = []
        for line in self.info_lines:
            if line.is_multi_line:
                chunks.extend(line.render())
            else:
                chunks.append(line.render())

        filtered_chunks = []
        for chunk in chunks:
            chunk = chunk.strip()
            if chunk:
                filtered_chunks.append(chunk)

        return filtered_chunks
