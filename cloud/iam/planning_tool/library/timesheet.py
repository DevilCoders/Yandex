# -*- coding: utf-8 -*-

import datetime

from cloud.iam.planning_tool.library.report import Report, Issue, Person


class TimesheetPerson(Person):
    def __init__(self, person_data):
        super().__init__(person_data)
        # из person_data используются поля:
        # ['name']['first']['ru']
        # ['name']['last']['ru']
        self.date_worklog: dict[datetime.date, DateWorklog] = {}
        # общее время, потраченное на задачу
        self.issue_total_time_spent: list[TimesheetIssue] = []
        self.raw_issue_total_time_spent: list[TimesheetIssue] = []

    # заполняет issue_total_time_spent и raw_issue_total_time_spent
    def fill_issue_time_spent(self):
        issue_total_time_spent = {}
        raw_issue_total_time_spent = {}
        for date_worklog in self.date_worklog.values():
            for issue in date_worklog.issues:
                issue_total_time_spent.setdefault(
                    issue.issue_data['key'],
                    TimesheetIssue(issue.issue_data, datetime.timedelta())).spent += issue.spent
            for issue in date_worklog.raw_issues:
                raw_issue_total_time_spent.setdefault(
                    issue.issue_data['key'],
                    TimesheetIssue(issue.issue_data, datetime.timedelta())).spent += issue.spent
        self.issue_total_time_spent = sorted(issue_total_time_spent.values(), key=lambda x: x.spent, reverse=True)
        self.raw_issue_total_time_spent = sorted(raw_issue_total_time_spent.values(),
                                                 key=lambda x: x.spent,
                                                 reverse=True)


class DateWorklog:
    def __init__(self, absence_type):
        self.absence_type = absence_type  # None, если не было отсутствия
        self.raw_total_time = None
        self.total_time = None
        self.raw_issues: list[TimesheetIssue] = []  # список TimesheetIssue
        self.issues: list[TimesheetIssue] = []  # список TimesheetIssue


class TimesheetIssue(Issue):
    def __init__(self, issue_data, spent):
        super().__init__(issue_data)
        self.spent = spent


class Timesheet(Report):
    template_name = 'timesheet.html'
    mds_path = 'reports.timesheet.mds.path'

    def __init__(self, config):
        super(Timesheet, self).__init__(config, 'timesheet')

        self._render_params = {'day_start': self._day_start,
                               'day_length': self._day_length,
                               'timezone': self._timezone,
                               'startrek_url': self._startrek_url,
                               'staff_url': self._staff_url,
                               'center_url': self._center_url}

    def prepare(self, from_date, to_date):
        persons = {person_data['login']: person_data for person_data in self._config.staff.persons(self._team)}
        issue_keys = set()
        absences = self._config.absence_util.work_days_interval_absences(from_date, to_date)
        for user in self._team:
            for wl in self._config.startrek.worklog_by_person(user, absences.from_date, absences.to_date):
                issue_keys.add(wl['issue']['key'])

        worklog = []
        for key in issue_keys:
            worklog.extend(self._config.startrek.worklog(key))
        worklog = self._config.worklog_util.normalize_worklog(worklog, absences, from_date, to_date)
        issues = {key: self._config.startrek.find(key, TimesheetIssue.get_fields()) for key in issue_keys}
        # Make date -> holiday
        #      user -> date -> DateWorklog
        # login -> TimesheetPerson
        user_worklog: dict[str, TimesheetPerson] = {}
        for login, person_data in persons.items():
            user_worklog[login] = TimesheetPerson(person_data)
        dates: dict[datetime.date, bool] = dict()  # является ли день выходным
        date = from_date
        while date < to_date:
            dates[date] = self._config.holidays.is_holiday(date)
            for user in user_worklog:
                user_worklog[user].date_worklog[date] = DateWorklog(absences.find_absence(user, date))
                if user in worklog and date in worklog[user].dates:
                    date_worklog_src = worklog[user].dates[date]
                    date_worklog_dst = user_worklog[user].date_worklog[date]
                    date_worklog_dst.raw_total_time = date_worklog_src.raw_total_time
                    date_worklog_dst.total_time = date_worklog_src.total_time
                    date_worklog_dst.raw_issues = [TimesheetIssue(issues[key], spent) for (key, spent) in
                                                   date_worklog_src.raw_issues.items()]
                    date_worklog_dst.issues = [TimesheetIssue(issues[key], spent) for (key, spent) in
                                               date_worklog_src.issues.items()]

            date = date + datetime.timedelta(days=1)
        for timesheet_person in user_worklog.values():
            timesheet_person.fill_issue_time_spent()
        self._render_params.update({'dates': dates,
                                    'user_worklog': user_worklog})
