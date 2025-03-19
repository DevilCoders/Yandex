# -*- coding: utf-8 -*-

import datetime

from cloud.iam.planning_tool.library.clients import GapClient
from cloud.iam.planning_tool.library.holidays import HolidayCalendar
from cloud.iam.planning_tool.library.utils import parse_duration


class Absences:
    def __init__(self, absences, holidays: HolidayCalendar, day_length, from_date, to_date):
        self._absences = absences  # login -> лист отсутствий
        self._holidays = holidays
        self._day_length = day_length

        self.from_date = from_date
        self.to_date = to_date

    @staticmethod
    def _absence_type(absence):
        if absence['work_in_absence'] or not absence['full_day']:
            return None

        if absence['workflow'] == 'absence':
            return 'duty-off' if '#duty' in absence['comment'] else 'other'
        elif absence['workflow'] == 'paid_day_off':
            return 'off'
        elif absence['workflow'] == 'vacation':
            return 'vacation'
        elif absence['workflow'] == 'illness':
            return 'illness'

        return 'other'

    # возвращает тип отсутствия по логину и дате
    def find_absence(self, user, date):
        if user in self._absences:
            for absence in self._absences[user]:
                if 'date_from' in absence and 'date_to' in absence:
                    absence_from = datetime.datetime.fromisoformat(absence['date_from']).date()
                    absence_to = datetime.datetime.fromisoformat(absence['date_to']).date()

                    if absence_from <= date < absence_to:
                        return self._absence_type(absence)

        return None

    def is_absence(self, user, date):
        return self.find_absence(user, date) is not None

    # совокупное время отсутствий у всех людей в указанный период
    def total_time(self, from_date, to_date):
        result = datetime.timedelta()
        for user in self._absences:
            for absence in self._absences[user]:
                if 'date_from' in absence and 'date_to' in absence:
                    absence_from = datetime.datetime.fromisoformat(absence['date_from']).date()
                    absence_to = datetime.datetime.fromisoformat(absence['date_to']).date()

                    date = absence_from
                    while date < absence_to:
                        date = date + datetime.timedelta(days=1)
                        if from_date <= date < to_date and self._holidays.is_work_day(date):
                            result = result + self._day_length

        return result


class AbsenceUtil:
    def __init__(self, config, gap: GapClient, holidays: HolidayCalendar):
        self._gap: GapClient = gap
        self._holidays: HolidayCalendar = holidays
        self._team = config['timesheet']['team']
        self._day_length = parse_duration(config['worklog'].get('day_length', '8h'))

    def absences(self, from_date, to_date) -> Absences:
        return Absences(self._gap.absences(from_date, to_date, self._team),
                        self._holidays, self._day_length,
                        from_date, to_date)

    # отсутствия в больший промежуток времени [min_date, max_date]
    # каждый человек проработал хотя бы один день в [min_date, to_date] и хотя бы один день в [to_date, max_date]
    def work_days_interval_absences(self, from_date, to_date) -> Absences:
        # ищем min_date и max_date - по ним в итоге вернем отсутствия
        # min_date
        absent_users = set(self._team)
        min_date = from_date
        load_min_date = from_date - datetime.timedelta(days=14)
        load_max_date = from_date
        # ставим min_date на такой день, что каждый человек поработал хотя бы один день в [min_date, from_date]
        while True:
            absences = self.absences(load_min_date, load_max_date)
            for user in list(absent_users):
                date = load_max_date
                # после цикла в date первый день перед load_max_date, когда user работал, или (load_min_date - 1)
                while date >= load_min_date:
                    if not absences.is_absence(user, date):
                        absent_users.remove(user)
                        break
                    date = date - datetime.timedelta(days=1)
                min_date = min(date, min_date)
            if len(absent_users) == 0:
                break
            load_max_date = load_min_date
            load_min_date = load_min_date - datetime.timedelta(days=14)

        # max_date
        absent_users = set(self._team)
        max_date = to_date
        load_min_date = to_date
        load_max_date = to_date + datetime.timedelta(days=14)
        # ставим max_date на такой день, что каждый человек поработал хотя бы один день в [to_date, max_date]
        while True:
            absences = self.absences(load_min_date, load_max_date)
            for user in list(absent_users):
                date = load_min_date
                # после цикла в date первый день после load_min_date, когда user работал, или (load_max_date + 1)
                while date <= load_max_date:
                    if not absences.is_absence(user, date):
                        absent_users.remove(user)
                        break
                    date = date + datetime.timedelta(days=1)
                max_date = max(date, max_date)
            if len(absent_users) == 0:
                break
            load_min_date = load_max_date
            load_max_date = load_max_date + datetime.timedelta(days=14)

        return self.absences(min_date, max_date)
