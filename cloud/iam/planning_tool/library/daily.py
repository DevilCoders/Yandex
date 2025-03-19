# -*- coding: utf-8 -*-

import datetime
import itertools
from collections import OrderedDict

from cloud.iam.planning_tool.library.report import Report, Issue, Person
from cloud.iam.planning_tool.library.utils import Node, collect_issue_subtree


class DailyPerson(Person):
    def __init__(self, person_data):
        super().__init__(person_data)
        # из person_data используются поля:
        # ['name']['first']['ru']
        # ['name']['last']['ru']
        self.norm_issues: dict[str, DailyIssue] = {}  # key задачи -> DailyIssue
        self.raw_issues: dict[str, DailyIssue] = {}  # key задачи -> DailyIssue

        self.norm_issues_list: list[DailyIssue] = []  # DailyIssue c нормализованным worklog
        self.raw_issues_list: list[DailyIssue] = []  # DailyIssue с ненормализованным worklog
        self.other_in_sprint_issues: list[DailyIssue] = []  # DailyIssue из спринта без worklog
        self.other_in_progress_issues: list[DailyIssue] = []  # DailyIssue не из спринта без worklog

        self.absences = {}  # дата -> отсутствие
        self.other_in_sprint_total = 0  # задачи из спринта, у которых нет worklog
        self.other_in_progress_total = 0  # задачи не из спринта, у которых нет worklog


class DailyIssue(Issue):
    def __init__(self, issue_data):
        super().__init__(issue_data)
        self.issue_data = issue_data  # словарь с описанием задачи
        # из issue_data используются поля:
        # ['id']
        # ['key']
        # ['status']['key']
        # ['status']['display']
        # ['summary']
        self.time_spent = {}  # дата -> затраченное время
        self.has_worklog = False
        self.in_sprint = False
        self.in_progress = False
        self.has_worklog_yesterday = False
        '''Field indicates if issue has worklog in the day of the report'''


class Timer:
    def __init__(self, timer: dict, team_size):
        self.meeting_length = self.get_seconds(timer.get('meeting_length', '00:30:00'))
        self.limit = self.get_seconds(timer.get('limit'))
        if self.limit is None:
            self.limit = round(self.meeting_length / team_size)
        self.size = timer.get('size', '200')
        self.key_moments = [self.get_seconds(time_moment) for time_moment in timer.get('key_moments', [])]
        self.key_moments = [moment for moment in self.key_moments if moment is not None]
        self.blink_duration = timer.get('blink_duration', '2000')
        self.blink_frequency = timer.get('blink_frequency', '3')
        self.blink_audio_src = timer.get('blink_audio_src')

    @staticmethod
    def get_seconds(time_str):
        if time_str is None:
            return None
        t = datetime.time.fromisoformat(time_str)
        return 3600 * t.hour + 60 * t.minute + t.second


class DailyReport(Report):
    template_name = 'daily.html'
    mds_path = 'reports.daily.mds.path'

    def __init__(self, config):
        super(DailyReport, self).__init__(config, 'daily')

        self._in_progress_filter = self._yaml['timesheet']['in_progress_filter']
        self._not_in_progress_statuses = self._yaml['timesheet']['not_in_progress_statuses']

        self._mds_path = self._yaml['reports']['daily']['mds']['path']

        DailyIssue.update_fields(['id',
                                  'key',
                                  'status',
                                  'summary',
                                  'assignee'])
        self._timer = Timer(self._yaml.get('timer', {}), len(self._team))
        self._render_params = {'day_start': self._day_start,
                               'day_length': self._day_length,
                               'timezone': self._timezone,
                               'startrek_url': self._startrek_url,
                               'staff_url': self._staff_url,
                               'center_url': self._center_url,
                               'timer': self._timer}

    def _get_active_sprint(self):
        sprints = self._config.startrek.sprints(self._board_id, DailyIssue.get_fields())
        sprint = None
        for item in sprints:
            if item['status'] == 'in_progress':
                if sprint is not None:
                    raise RuntimeError('Too many active sprints')
                sprint = item

        return sprint

    def _in_progress_query(self):
        temp = f'{self._in_progress_filter} AND assignee: {{}} AND {{}}'
        return temp.format(
            ','.join(map(lambda user: f'{user}@', self._team)),
            ' AND '.join(map(lambda status: f'Status: !"{status}"', self._not_in_progress_statuses)))

    def process_in_progress_issues(self, daily_person, issues_type, node_by_id: dict[str, Node]):
        """Устанавливает in_progress=False на те задачи, внутри которых есть подзадача или подэпик,
        по которым были отметки времени"""
        issues_dict: dict[str, DailyIssue] = getattr(daily_person, issues_type)
        for issue in issues_dict.values():
            collect_issue_subtree(issue.id, issue.key, node_by_id, self._config.startrek.links)
        # есть ли в поддереве задачи те, по которым были отметки времени по issue_id
        has_worklog_in_subtree: dict[int, bool] = {}

        # заполняет и возвращает has_worklog_in_subtree[issue.key]
        def get_has_worklog_in_subtree(issue: Node):
            if issue.id not in has_worklog_in_subtree:
                if issue.key in issues_dict and issues_dict[issue.key].has_worklog:
                    return True
                for child in node_by_id[issue.id].children:
                    if get_has_worklog_in_subtree(child):
                        has_worklog_in_subtree[issue.id] = True
                        break
                else:
                    has_worklog_in_subtree[issue.id] = False
            return has_worklog_in_subtree[issue.id]

        for issue in issues_dict.values():
            if get_has_worklog_in_subtree(node_by_id[issue.id]):
                issues_dict[issue.key].in_progress = False

    @staticmethod
    def _sort_issues_dict(issues_dict: dict[str, DailyIssue], condition, sort_key):
        return list(map(lambda x: x[1], sorted(filter(condition, issues_dict.items()), key=sort_key)))

    def prepare(self, date, shift):
        # вычисляем дни в отчете
        if self._config.holidays.is_work_day(date):
            today = date
        else:
            today = self._config.holidays.next_work_day(date)
        yesterday = self._config.holidays.previous_work_day(today)
        norm_days = [self._config.holidays.previous_work_day(yesterday), yesterday, today]
        raw_days = [date - datetime.timedelta(days=2), date - datetime.timedelta(days=1), date]
        norm_this_day = yesterday
        raw_this_day = date

        # получаем информацию о людях
        persons = {person['login']: person for person in self._config.staff.persons(self._team) if
                   not person['official']['is_dismissed']}
        # получаем информацию об отсутствиях
        absences = self._config.absence_util.work_days_interval_absences(norm_days[0], norm_days[2])
        # получаем задачи, у которых указано затраченное время в данные дни
        worklog_issues = self._get_worklog_issues(absences.from_date, absences.to_date, DailyIssue.get_fields())
        # получаем список отмеченного времени по каждой задаче
        worklog = []
        for issue in worklog_issues:
            worklog.extend(self._config.startrek.worklog(issue['key']))
        # нормализуем worklog
        # login -> UserWorklog
        worklog = self._config.worklog_util.normalize_worklog(worklog, absences, absences.from_date, absences.to_date)
        # получаем задачи из спринта
        sprint = self._get_active_sprint()
        sprint_issues = self._config.startrek.sprint_issues(sprint['id'])
        # получаем не закрытые задачи
        in_progress_issues = self._config.startrek.search(self._in_progress_query(), DailyIssue.get_fields())

        # задачи по ключу
        issues_by_key = {}
        for issue in itertools.chain(worklog_issues, sprint_issues, in_progress_issues):
            issues_by_key[issue['key']] = issue
        # список использований задач, у которых есть worklog
        usages_by_key: dict[str, list[DailyIssue]] = {}
        # люди по логину
        persons_by_login: dict[str, DailyPerson] = {}

        # обрабатываем задачи с отмеченным временем, добавляем их в usages_by_key и persons_by_login
        def process_login(login, issues_type):
            norm_issues = (issues_type == 'norm_issues')
            worklog_issues_type = 'issues' if norm_issues else 'raw_issues'
            days = norm_days if norm_issues else raw_days
            for date in filter(lambda day: day in worklog[login].dates, days):
                date_worklog = worklog[login].dates[date]
                for key, time_spent in getattr(date_worklog, worklog_issues_type).items():
                    issues_dict = getattr(persons_by_login[login], issues_type)
                    if key not in issues_dict:
                        issues_dict[key] = DailyIssue(issues_by_key[key])
                        # дополняем список использований задачи key
                        usages_by_key.setdefault(key, []).append(issues_dict[key])
                    issue = issues_dict[key]
                    issue.time_spent[date] = time_spent
                    issue.has_worklog = True
                    if date == yesterday:
                        issue.has_worklog_yesterday = True

        for login, person in persons.items():
            persons_by_login[login] = DailyPerson(person)
            for date in norm_days:
                absence = absences.find_absence(login, date)
                if absence is not None:
                    persons_by_login[login].absences[date] = absence
            if login in worklog:
                process_login(login, 'norm_issues')
                process_login(login, 'raw_issues')

        # обрабатываем задачи в спринте, добавляем их в persons_by_login
        for issue in sprint_issues:
            key = issue['key']
            # добавляем in_sprint=True ко всем использованиям issue с отмеченным временем
            for daily_issue in usages_by_key.get(key, []):
                daily_issue.in_sprint = True
            # добавляем задачу к исполнителю
            if issue.get('assignee') is not None and issue['assignee']['id'] in persons_by_login:
                daily_person = persons_by_login[issue['assignee']['id']]
                daily_person.norm_issues.setdefault(key, DailyIssue(issue)).in_sprint = True
                daily_person.raw_issues.setdefault(key, DailyIssue(issue)).in_sprint = True

        # добавляем остальные задачи
        for issue in in_progress_issues:
            if issue.get('assignee') is not None and issue['assignee']['id'] in persons_by_login:
                key = issue['key']
                daily_person = persons_by_login[issue['assignee']['id']]
                daily_person.norm_issues.setdefault(key, DailyIssue(issue)).in_progress = True
                daily_person.raw_issues.setdefault(key, DailyIssue(issue)).in_progress = True
        node_by_id: dict[str, Node] = {}
        for daily_person in persons_by_login.values():
            self.process_in_progress_issues(daily_person, 'norm_issues', node_by_id)
            self.process_in_progress_issues(daily_person, 'raw_issues', node_by_id)

        # ключи для сортировки задач
        # в конце те, у кого не отмечено время вчера
        # в конце те, у кого не отмечено время позавчера и сегодня
        # далее сортировка по key задачи
        def norm_issue_sort_key(kv):
            time_spent = kv[1].time_spent
            return [norm_days[1] not in time_spent,
                    norm_days[0] not in time_spent and norm_days[2] not in time_spent,
                    kv[0]]

        def raw_issue_sort_key(kv):
            time_spent = kv[1].time_spent
            return [raw_days[1] not in time_spent,
                    raw_days[0] not in time_spent and raw_days[2] not in time_spent,
                    kv[0]]

        def other_issue_sort_key(kv):
            return kv[0]

        # сортируем задачи и заполняем 4 списка задач в DailyIssue
        for daily_person in persons_by_login.values():
            daily_person.norm_issues_list = self._sort_issues_dict(
                daily_person.norm_issues,
                lambda kv: kv[1].has_worklog,
                norm_issue_sort_key
            )
            daily_person.raw_issues_list = self._sort_issues_dict(
                daily_person.raw_issues,
                lambda kv: kv[1].has_worklog,
                raw_issue_sort_key
            )
            daily_person.other_in_sprint_issues = self._sort_issues_dict(
                daily_person.norm_issues,
                lambda kv: not kv[1].has_worklog and kv[1].in_sprint,
                other_issue_sort_key
            )
            daily_person.other_in_progress_issues = self._sort_issues_dict(
                daily_person.norm_issues,
                lambda kv: not kv[1].has_worklog and not kv[1].in_sprint and kv[1].in_progress,
                other_issue_sort_key
            )

        # подсчитываем число задач в каждой категории
        for daily_person in persons_by_login.values():
            daily_person.other_in_sprint_total = len(daily_person.other_in_sprint_issues)
            daily_person.other_in_progress_total = len(daily_person.other_in_progress_issues)

        # sorting persons by logins
        temp_persons_by_login = sorted(persons_by_login.items(), key=lambda kv: kv[0])
        if shift:
            shift = today.toordinal() % len(temp_persons_by_login)
            temp_persons_by_login = temp_persons_by_login[shift:] + temp_persons_by_login[:shift]

            # в начале те, у кого есть задачи с отмеченным временем
            temp_persons_by_login.sort(key=lambda kv: (len([*filter(lambda x: x.has_worklog_yesterday,
                                                                    kv[1].norm_issues_list)]) != 0),
                                       reverse=True)
        # moving persons, absent today, to the end
        temp_persons_by_login.sort(key=lambda x: today in x[1].absences)
        persons_by_login = OrderedDict(temp_persons_by_login)

        self._render_params |= {'norm_days': norm_days,
                                'raw_days': raw_days,
                                'norm_this_day': norm_this_day,
                                'raw_this_day': raw_this_day,
                                'issues': persons_by_login}
