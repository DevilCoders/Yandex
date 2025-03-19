# -*- coding: utf-8 -*-

import datetime
from collections import OrderedDict
from typing import Optional, Callable, List, Dict

from cloud.iam.planning_tool.library.report import Report, Issue, TaskTree
from cloud.iam.planning_tool.library.utils import Node, parse_duration, styled_print, styled_reprint


class RenderFieldsCongig:
    __slots__ = ('time_after', 'time_total', 'priority', 'time_remaining', 'time_orig_estimated',
                 'progress_bar', 'time_ratio')

    def __init__(self, hidden_fields, time_spent_criteria):
        if time_spent_criteria is not None:
            for slot in self.__slots__:
                setattr(self, slot, slot.replace not in hidden_fields)
            self.time_after &= time_spent_criteria == 'worklog'
        else:
            for slot in self.__slots__:
                setattr(self, slot, False)
            self.priority = 'priority' not in hidden_fields

    def keys(self):
        return map(lambda x: f'show_{x}', self.__slots__)

    def __getitem__(self, item):
        return getattr(self, item.removeprefix('show_'))


class MonthlyIssue(Issue):
    """
    Attributes:
        category : путь по категориям к задаче
        time_spent: затрачено в указанные пользователем сроки
    """

    comment_prefix = '#monthly'

    cache: dict[int, 'MonthlyIssue'] = {}

    _fields = {'id', 'key', 'status', 'summary', 'epic', 'parent', 'links', 'tags', 'importance', 'storyPoints',
               'originalStoryPoints', 'sprint', 'spentSp'}

    def __init__(self, issue_data=None):
        if issue_data is None:
            self.key = None
            self.tags = []
            self.importance = None
            self.issue_data = {}
            return
        else:
            super().__init__(issue_data)

        self.sub_issues: dict[int, MonthlyIssue] = {}

        self.worklog = None
        self.category: Optional[list[str]] = None
        '''путь по категориям к задаче'''

        self.new_tag = None
        self.previous_tag = None

        self.time_spent = None
        '''затрачено в указанные пользователем сроки'''

        self.total_time_spent = None
        '''затрачено всего'''

        self.raw_time_spent = None
        '''без учета выходных и начала рабочего дня'''

        self.raw_total_time_spent = None
        '''без учета выходных и начала рабочего дня'''

        self.time_original_estimated = None
        self.time_remaining = None
        self.remaining_tasks = None

    def set_time_spent(self,
                       raw_time_spent,
                       raw_total_time_spent,
                       time_spent,
                       total_time_spent,
                       time_remaining,
                       remaining_tasks):
        self.raw_time_spent = raw_time_spent
        self.raw_total_time_spent = raw_total_time_spent
        self.time_spent = time_spent
        self.total_time_spent = total_time_spent
        self.time_remaining = time_remaining
        self.remaining_tasks = remaining_tasks

    def add_time_spent(self, other: 'MonthlyIssue'):
        self.raw_time_spent += other.raw_time_spent
        self.raw_total_time_spent += other.raw_total_time_spent
        self.time_spent += other.time_spent
        self.total_time_spent += other.total_time_spent
        self.time_remaining += other.time_remaining
        self.remaining_tasks += other.remaining_tasks

    def has_tags(self, new_tag, previous_tag):
        if not self.tags:
            return False

        if new_tag is not None:
            return new_tag[0] in self.tags

        if previous_tag is not None:
            return previous_tag[0] in self.tags

        return False

    def fill_tags(self, new_tag, previous_tag):
        """
        заполняет issue.new_tag и issue.previous_tag, если new_tag и previous_tag есть в issue.tags
        :param new_tag:
        :param previous_tag:
        :return:
        """
        if self.tags:
            if new_tag is not None and new_tag[0] in self.tags:
                self.new_tag = new_tag[1]
            if previous_tag is not None and previous_tag[0] in self.tags:
                self.previous_tag = previous_tag[1]

    def original_story_points_time_spent(self, to_timedelta_func: Callable[[int], datetime.timedelta]):
        raw_spent = raw_total_spent = spent = total_spent = remaining = to_timedelta_func(self.spentSp)
        if self.originalStoryPoints is not None:
            remaining = to_timedelta_func(max(self.originalStoryPoints - self.spentSp, 0))

        remaining_tasks = [self] if remaining != datetime.timedelta() else []

        return [raw_spent, raw_total_spent, spent, total_spent, remaining, remaining_tasks]

    def story_points_time_spent(self, to_timedelta_func: Callable[[int], datetime.timedelta]):
        value = datetime.timedelta()

        if self.originalStoryPoints is not None:
            value = to_timedelta_func(self.originalStoryPoints)
        elif self.storyPoints is not None:
            value = to_timedelta_func(self.storyPoints)

        raw_spent = raw_total_spent = spent = total_spent = remaining = value

        remaining_tasks = [self] if value != datetime.timedelta() else []

        return [raw_spent, raw_total_spent, spent, total_spent, remaining, remaining_tasks]

    def worklog_time_spent(self, worklog, from_date, to_date,
                           to_timedelta_func: Callable[[int], datetime.timedelta]):
        raw_spent = datetime.timedelta()
        raw_total_spent = datetime.timedelta()
        spent = datetime.timedelta()
        total_spent = datetime.timedelta()
        key = self.key
        for user in worklog:
            for date in worklog[user].dates:
                if key in worklog[user].dates[date].issues:
                    total_spent += worklog[user].dates[date].issues[key]
                    if from_date <= date < to_date:
                        spent += worklog[user].dates[date].issues[key]

                if key in worklog[user].dates[date].raw_issues:
                    raw_total_spent += worklog[user].dates[date].raw_issues[key]
                    if from_date <= date < to_date:
                        raw_spent += worklog[user].dates[date].raw_issues[key]
        remaining = to_timedelta_func(self.storyPoints) if self.status != 'closed' else datetime.timedelta()
        remaining_tasks = [self] if remaining != datetime.timedelta() else []
        return [raw_spent, raw_total_spent, spent, total_spent, remaining, remaining_tasks]

    def init_time_spent(self, worklog, from_date, to_date, time_spent_criteria: str,
                        to_timedelta_func: Callable[[int], datetime.timedelta]):
        result = []
        if time_spent_criteria == 'worklog':
            result = self.worklog_time_spent(worklog, from_date, to_date, to_timedelta_func)
        elif time_spent_criteria == 'originalStoryPoints':
            result = self.original_story_points_time_spent(to_timedelta_func)
        elif time_spent_criteria == 'storyPoints':
            result = self.story_points_time_spent(to_timedelta_func)
        else:
            raise ValueError('Incorrect value of field "time_spent" in config')

        self.set_time_spent(*result)

    def fill_time_spent(self, worklog, from_date, to_date, time_spent_criteria: str,
                        to_timedelta_func: Callable[[int], datetime.timedelta]):
        """
        Рекурсивно заполняет время для задач
        """
        if time_spent_criteria is None:
            return
        if self.time_spent is not None:
            print(f'It seems that {self.key} has two parent')
            return

        if self.originalStoryPoints is not None:
            self.time_original_estimated = to_timedelta_func(self.originalStoryPoints)

        self.init_time_spent(worklog, from_date, to_date,
                             time_spent_criteria,
                             to_timedelta_func)

        for sub_key, sub_issue in self.sub_issues.items():
            sub_issue.fill_time_spent(worklog, from_date, to_date, time_spent_criteria, to_timedelta_func)
            self.add_time_spent(sub_issue)

    def sort_key(self, top_level, new_tag, previous_tag):
        """
        ключ для сортировки TaskTree.issues
            [issue.importance,
            new_tag[0] in tags and previous_tag[0] not in tags,
            previous_tag[0] not in tags,
            issue.key]

        по убыванию issue.importance
        далее сначала те, у кого new_tag[0] not in tags or previous_tag[0] in tags
        далее сначала те, у кого previous_tag[0] in tags
        далее сортировка по issue.key
        """
        # подзадачи сортируем по названию тикета
        if not top_level:
            return self.parse_key()

        return (self.priority,
                new_tag is None or (new_tag[0] in self.tags and
                                    (previous_tag is None or previous_tag[0] not in self.tags)),
                previous_tag is None or previous_tag[0] not in self.tags,
                self.parse_key())

    @staticmethod
    def get_last_comment(issue_id: str, st_client) -> Optional[str]:
        '''
        Запрашивает комментарии у трекера и возвращает последний комментарий,
        начинающийся с MonthlyIssue.comment_prefix
        '''
        get_response: List[Dict[str, any]] = MonthlyIssue.get_comments(issue_id, st_client)
        for i in range(len(get_response) - 1, -1, -1):
            comment = get_response[i]
            if comment['text'].startswith(MonthlyIssue.comment_prefix):
                last_comment = MonthlyIssue.get_html_comment(issue_id, comment['id'], st_client)
                if last_comment is None:
                    return None

                prefix_index = last_comment.index(MonthlyIssue.comment_prefix)
                # MonthlyIssue.comment_prefix не обязательно в начале html кода
                return last_comment[:prefix_index] + last_comment[prefix_index + len(MonthlyIssue.comment_prefix):]


class MonthlyTaskTree(TaskTree):
    def __init__(self, *, collapsed=None, visible=None, chart=None, reverse_priority, collapse_equal_priority):
        super().__init__(collapsed=collapsed,
                         visible=visible,
                         chart=chart,
                         reverse_priority=reverse_priority,
                         collapse_equal_priority=collapse_equal_priority)
        self.time_spent = datetime.timedelta()
        self.raw_time_spent = datetime.timedelta()
        self.total_time_spent = datetime.timedelta()
        self.raw_total_time_spent = datetime.timedelta()

        self.chart = chart
        """указывает, будет ли показан чарт по категории, по умолчанию True"""

        self.ratio = None
        self.raw_ratio = None

    def sort(self, category, is_category, new_tag, previous_tag):
        # сортируем self.items
        # значение для сортировки - номер первого вхождения ключа в category['items']
        # т. е. порядок как в config
        self.items = OrderedDict(sorted(self.items.items(),
                                        key=lambda kv: next(
                                            i for i, elem in enumerate(category['items']) if elem['name'] == kv[0])))
        for category_name, task_tree in self.items.items():
            # вместо category подставляем подкатегорию с именем key
            if isinstance(task_tree, MonthlyTaskTree):  # checking only for typehints
                task_tree.sort(next(iter(filter(lambda x: x['name'] == category_name, category['items']))),
                               True, new_tag, previous_tag)

        # сортируем self.issues

        if is_category:
            self.prioritize_by_field()
        self.issues = sorted(self.issues, key=lambda x: x.sort_key(is_category, new_tag, previous_tag))

    def categorize_issue(self, issue: MonthlyIssue, categories):
        """добавляет issue в task_tree по пути, заполняем время в task_tree"""
        last = self
        last.add_time_spent(issue)

        category = {'items': categories}
        # перебираем путь к категории
        for category_key in issue.category:
            # категория в category['items'], равная category_key
            category = next(iter(filter(lambda x: x['name'] == category_key, category['items'])))
            if category_key not in last.items:
                last.items[category_key] = type(self)(collapsed=category.get('collapsed', False),
                                                      visible=category.get('visible', True),
                                                      chart=category.get('chart', True),
                                                      reverse_priority=self.reverse_priority,
                                                      collapse_equal_priority=self.collapse_equal_priority)

            last = last.items[category_key]
            last.add_time_spent(issue)

        last.issues.append(issue)

    def calculate_time_ratio(self):
        """
        Считает долю затраченного времени на подкатегории и задачи
        """
        for name, c in self.items.items():
            if self.time_spent != datetime.timedelta():
                c.ratio = c.time_spent / self.time_spent
            else:
                c.ratio = 0
            if self.raw_time_spent != datetime.timedelta():
                c.raw_ratio = c.raw_time_spent / self.raw_time_spent
            else:
                c.raw_ratio = 0
            c.calculate_time_ratio()

    def add_time_spent(self, issue: 'MonthlyIssue'):
        try:
            self.raw_time_spent += issue.raw_time_spent
            self.time_spent += issue.time_spent
            self.total_time_spent += issue.total_time_spent
            self.raw_total_time_spent += issue.raw_total_time_spent
        except:
            pass


class MonthlyReport(Report):
    template_name = 'monthly.html'
    mds_path = 'reports.monthly.mds.path'

    def __init__(self, config):
        super(MonthlyReport, self).__init__(config, 'monthly')
        self._categories = self._yaml['timesheet']['categories']
        self._mds_path = self._yaml['reports']['monthly']['mds']['path']
        MonthlyIssue.update_fields(self._yaml['reports']['monthly']['additional_fields'])
        self.prioritizing_cfg = {}
        if 'prioritize' in self._yaml['reports']['monthly']:
            self.prioritizing_cfg = prioritizing_cfg = self._yaml['reports']['monthly']['prioritize']
            MonthlyIssue.update_fields((prioritizing_cfg['field'],))
            MonthlyIssue._priority_field = prioritizing_cfg['field']

        self._sp_time = parse_duration(self._yaml['worklog'].get('sp_time', '8h'))
        self._time_spent_criteria = self._yaml['worklog']['time_spent']
        self.task_tree: Optional[TaskTree] = None

        try:
            hidden_fields = {*self._yaml['render_config']['monthly']['hidden']}
        except (KeyError, TypeError):
            hidden_fields = set()
        if 'prioritize' not in self._yaml['reports']['monthly']:
            hidden_fields.add('priority')

        fields_config = RenderFieldsCongig(hidden_fields, self._time_spent_criteria)

        self._render_params = {
            'day_start': self._day_start,
            'day_length': self._day_length,
            'timezone': self._timezone,
            'startrek_url': self._startrek_url,
            **fields_config
        }

        show_additional_information = self._time_spent_criteria != 'storyPoints'

        self._render_params['show_time_remaining'] = show_additional_information
        self._render_params['show_progress_bar'] = show_additional_information
        self._render_params['show_time_orig_estimated'] = show_additional_information

    def _to_timedelta(self, sp):
        # 1 sp = self._sp_time hours
        return datetime.timedelta(seconds=sp * self._sp_time.total_seconds())

    @staticmethod
    def _parse_tag(tag):
        if tag is None:
            return None

        parts = tag.split(' ')

        if len(parts) == 1:
            return parts[0].strip(), parts[0].strip()

        if len(parts) == 2:
            return parts[0].strip(), parts[1].strip()

        raise RuntimeError('Invalid tag format')

    def prepare(self, from_date, to_date, new_tag, previous_tag):
        new_tag = self._parse_tag(new_tag)
        previous_tag = self._parse_tag(previous_tag)

        # Find all issues with worklog in from_date..to_date
        if self._time_spent_criteria == 'worklog':
            raw_issues = self._get_worklog_issues(from_date, to_date, MonthlyIssue.get_fields())
        else:
            sprints = self._config.startrek.sprints(self._board_id, MonthlyIssue.get_fields())
            raw_issues: list[dict] = []
            for sprint in sprints:
                start = datetime.date.fromisoformat(sprint['startDate'])
                end = datetime.date.fromisoformat(sprint['endDate'])
                if end >= from_date and start <= to_date:
                    raw_issues.extend(self._config.startrek.sprint_issues(sprint['id']))

        MonthlyIssue.create_many(raw_issues)

        # Add all parent issues and every their child to calculate the total time spent for every issue at the top level
        styled_print('[-] Collecting related issues')

        group_issues_by_key: dict[int, MonthlyIssue] = {}  # top_level_issue_key -> top_level_issue
        nodes_by_id: dict[int, Node] = {}
        for issue_id, issue in list(MonthlyIssue.cache.items()):
            path: list[MonthlyIssue] = []
            group_issue: MonthlyIssue = issue.get_group_issue(self._config.startrek, path)
            group_issues_by_key[group_issue.key] = group_issue
            if self._time_spent_criteria == 'worklog':
                group_issue.collect_issues(nodes_by_id, self._config.startrek)
            else:
                for child, parent in zip(path[:-1], path[1:]):
                    parent.sub_issues[child.key] = child
            styled_reprint(f'[-] Collecting related issues ({len(MonthlyIssue.cache)} issues collected)')

        styled_reprint(f'[+] Collecting related issues ({len(MonthlyIssue.cache)} issues collected)')

        # In order to calculate the normalized time we need to load all issues with worklog in every worklog date
        # from the previous issue list
        # Note that we need to take into account that min_date cannot be a holiday or any kind of absence since not all
        # worklogs from the start
        # of the day are loaded otherwise. The same applies to the max_date value.
        absences = self._config.absence_util.work_days_interval_absences(from_date, to_date)

        if absences.from_date < from_date or absences.to_date > to_date:
            raw_issues = []
            for user in self._team:
                for wl in self._config.startrek.worklog_by_person(user, absences.from_date, absences.to_date):
                    raw_issues.append(self._config.startrek.find(wl['issue']['id'], MonthlyIssue.get_fields()))
            MonthlyIssue.create_many(raw_issues)

        # Load issues worklogs
        if self._time_spent_criteria == 'worklog':
            print()
            min_date = min(absences.from_date, from_date)
            max_date = max(absences.to_date, to_date)
            worklog = []

            for issue_id, issue in MonthlyIssue.cache.items():
                wl = self._config.startrek.worklog(issue.key)
                worklog.extend(wl)
                issue.worklog = wl

                for item in wl:
                    start = datetime.datetime.strptime(item['start'], '%Y-%m-%dT%H:%M:%S.%f%z').date()
                    if start < min_date:
                        min_date = start
                    if start > max_date:
                        max_date = start

            worklog = self._config.worklog_util.normalize_worklog(worklog, absences, min_date, max_date)
        else:
            worklog = None

        self._delete_not_existent_categories({'items': self._categories})

        system_categories = {}
        self._fill_system_categories({'items': self._categories}, [], system_categories)

        task_tree = MonthlyTaskTree(reverse_priority=self.prioritizing_cfg.get('reverse', False),
                                    collapse_equal_priority=self.prioritizing_cfg.get('collapse', False))
        for issue in group_issues_by_key.values():
            # заполняем рекурсивно время у задач
            issue.fill_time_spent(worklog, from_date, to_date, self._time_spent_criteria, self._to_timedelta)

            if any((issue.time_spent != datetime.timedelta(),
                    issue.raw_time_spent != datetime.timedelta(),
                    issue.has_tags(new_tag, previous_tag),
                    self._time_spent_criteria is None)):
                # заполняем путь по категориям к задаче
                issue.category = issue.get_category(system_categories, self._categories)
                # заполняем issue.new_tag и issue.previous_tag
                issue.fill_tags(new_tag, previous_tag)
                if issue.category is not None:
                    # добавляем issue в task_tree
                    task_tree.categorize_issue(issue, self._categories)

        if 'absence' in system_categories:
            # добавляем категорию absence
            absence_time = absences.total_time(from_date, to_date)
            absence_issue = MonthlyIssue()
            absence_issue.category = system_categories['absence']
            absence_issue.time_spent = absence_time
            absence_issue.raw_time_spent = absence_time
            # добавляем absence_issue в task_tree
            task_tree.categorize_issue(absence_issue, self._categories)
        # считаем доли затраченного времени на категории и подзадачи
        task_tree.calculate_time_ratio()
        task_tree.filter_issues(lambda x: x.key is not None and any((x.time_spent != datetime.timedelta(),
                                                                     x.new_tag is not None,
                                                                     x.previous_tag is not None,
                                                                     self._time_spent_criteria is None)))
        task_tree.sort({'items': self._categories}, False, new_tag, previous_tag)

        # добавляем комментарии к задачам
        styled_print('[-] Adding comments')
        for issue in MonthlyIssue.cache.values():
            last_comment = MonthlyIssue.get_last_comment(issue.key, self._config.startrek)
            issue.comment = '' if last_comment is None else last_comment
        styled_reprint('[+] Adding comments')

        self._render_params['task_tree'] = task_tree
