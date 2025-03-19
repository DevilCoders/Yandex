# -*- coding: utf-8 -*-

import datetime
import isodate
import pytz

from cloud.iam.planning_tool.library.utils import parse_duration


class UserWorklog:
    def __init__(self):
        # date -> DateWorklog (общее затраченное время в этот день и затраченное время по key задачи)
        self.dates: dict[datetime.date, DateWorklog] = {}
        self.issues: dict[str, IssueWorklog] = {}  # key задачи ->  IssueWorklog
        self.raw_issues: dict[str, IssueWorklog] = {}  # key задачи -> IssueWorklog


class DateWorklog:
    def __init__(self):
        self.raw_total_time = datetime.timedelta()
        self.total_time = datetime.timedelta()
        self.raw_issues: dict[str, datetime.timedelta] = {}  # key задачи -> затраченное время
        self.issues: dict[str, datetime.timedelta] = {}  # key задачи -> затраченное время


class IssueWorklog:
    def __init__(self):
        self.total_time = datetime.timedelta()
        self.dates: [datetime.date, DayWorklog] = {}  # date -> DayWorklog

    def add_day_worklog(self, day, duration, wl):
        self.total_time += duration
        day_worklog = self.dates.setdefault(day, DayWorklog())
        day_worklog.total_time += duration
        day_worklog.worklog.append(wl)


class DayWorklog:
    def __init__(self):
        self.total_time = datetime.timedelta()
        self.worklog = []  # список словарей - описания worklog


class WorklogUtil:
    def __init__(self, config, holidays):
        self._timezone = pytz.timezone(config['timezone'])
        self._day_start = datetime.time.fromisoformat(config['day_start'])
        self._day_length = parse_duration(config.get('day_length', '8h'))

        self._holidays = holidays

    def _parse_worklog_duration(self, duration):
        # 1 day = 8 hours
        result = isodate.parse_duration(duration)
        return datetime.timedelta(seconds=result.days * self._day_length.total_seconds() + result.seconds)

    def _raw_work_day(self, ts):
        return ts.astimezone(self._timezone).date()

    def _normalize_work_day(self, ts, absences, user):
        """Возвращает первый рабочий день (дни начинаются с self._day_start) до ts"""
        local = ts.astimezone(self._timezone)
        if local.time() >= self._day_start:
            date = local.date()
        else:
            date = (local - datetime.timedelta(days=1)).date()

        while self._holidays.is_holiday(date) or (absences is not None and absences.is_absence(user, date)):
            date -= datetime.timedelta(days=1)

        return date

    def normalize_worklog(self, worklog, absences=None, from_date=None, to_date=None) -> dict[str, UserWorklog]:
        result = dict()  # login -> UserWorklog
        for wl in worklog:
            updated_by = wl['updatedBy']['id']
            key = wl['issue']['key']
            duration = self._parse_worklog_duration(wl['duration'])

            start = datetime.datetime.strptime(wl['start'], '%Y-%m-%dT%H:%M:%S.%f%z')
            user_worklog = result.setdefault(updated_by, UserWorklog())

            day = self._raw_work_day(start)
            if not (from_date and to_date and (day < from_date or day >= to_date)):
                date_worklog = user_worklog.dates.setdefault(day, DateWorklog())
                date_worklog.raw_total_time += duration
                date_worklog.raw_issues[key] = date_worklog.raw_issues.get(key, datetime.timedelta()) + duration

                issue_worklog = user_worklog.raw_issues.setdefault(key, IssueWorklog())
                issue_worklog.add_day_worklog(day, duration, wl)

            day = self._normalize_work_day(start, absences, updated_by)
            if not (from_date and to_date and (day < from_date or day >= to_date)):
                date_worklog = user_worklog.dates.setdefault(day, DateWorklog())
                date_worklog.total_time += duration
                date_worklog.issues[key] = date_worklog.issues.get(key, datetime.timedelta()) + duration

                issue_worklog = user_worklog.issues.setdefault(key, IssueWorklog())
                issue_worklog.add_day_worklog(day, duration, wl)

        for user in result:
            # нормализуем DateWorklog
            for date in result[user].dates:
                date_worklog = result[user].dates[date]
                k = self._day_length / date_worklog.total_time if date_worklog.total_time != datetime.timedelta() else 1
                total_time = datetime.timedelta()
                for key in date_worklog.issues:
                    wl = date_worklog.issues[key]
                    date_worklog.issues[key] = wl * k
                    total_time += wl

                result[user].dates[date].total_time = total_time * k

            # нормализуем IssueWorklog
            for issue in result[user].issues:
                total_time = datetime.timedelta()
                for date in result[user].issues[issue].dates:
                    wl = result[user].dates[date].issues[issue]
                    result[user].issues[issue].dates[date].total_time = wl
                    total_time += wl

                result[user].issues[issue].total_time = total_time

        return result
