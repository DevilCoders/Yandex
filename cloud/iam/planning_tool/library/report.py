import abc
import datetime
import functools
import pickle
import sys
from collections import OrderedDict
from typing import Optional, Iterable, TypeVar, Type, Callable

import pytz
import requests
from bs4 import BeautifulSoup
from requests.exceptions import HTTPError

from cloud.iam.planning_tool.library.clients import StartrekClient
from cloud.iam.planning_tool.library.config import Config
from cloud.iam.planning_tool.library.utils import parse_duration, Node, get_or_load_issue_node, styled_print, \
    styled_reprint

T = TypeVar('T', bound='Issue')


class Person:
    def __init__(self, person_data: dict):
        self.person_data = person_data


class Issue:
    _fields: set[str] = {'id', 'key', 'status', 'summary'}

    _priority_field = None

    cache: dict['str', 'Issue'] = {}
    """Storage for all issues"""

    @classmethod
    def get_fields(cls):
        return {*cls._fields}

    @classmethod
    def update_fields(cls, values: Iterable[str]):
        cls._fields = cls._fields.union(values)

    def __init__(self, issue_data: dict):
        """
        :param issue_data: словарь с описанием тикета
        """

        # из issue_data используются поля:
        # ['status']['key']
        # ['status']['display']
        # ['summary']
        # ['id']
        # ['key']
        # ['epic']['id']
        # ['parent']['id']
        # ['storyPoints']
        # ['originalStoryPoints']
        # ['tags']
        # ['importance']
        self._priority = None
        self.category = None  # list, путь по категориям к задаче
        self.top_level_issue = None
        self.sub_issues_filled = False
        self.sub_issues: dict[str, 'Issue'] = {}
        self.issue_data = issue_data
        self.tags = issue_data.get('tags', [])
        self.key = issue_data['key']
        self.id = issue_data['id']
        self.importance = issue_data.get('importance')
        self.epic_key = None if issue_data.get('epic') is None else issue_data['epic']['key']
        self.links = issue_data.get('links', None)
        self.remote_links = issue_data.get('remotelinks', None)
        self.parent_key = None if issue_data.get('parent') is None else issue_data['parent']['key']
        self.summary = issue_data['summary']
        self.originalStoryPoints = issue_data.get('originalStoryPoints')
        self.status = issue_data['status']['key']
        self.storyPoints = 0 if issue_data.get('storyPoints') is None else issue_data['storyPoints']
        self.spentSp = 0 if issue_data.get('spentSp') is None else issue_data['spentSp']
        self._top_level_issue: Optional[Issue] = None

        self.cache[self.key] = self

    def __getattr__(self, item):
        if item in self.issue_data:
            return self.issue_data[item]
        if item in self.get_fields():
            return None
        raise AttributeError(f'{type(self).__name__} object has no field {item}')

    @classmethod
    def create_many(cls, issue_datas: Iterable[dict]):
        for issue_data in issue_datas:
            if issue_data['key'] not in cls.cache:
                cls(issue_data)

    @functools.lru_cache
    def parse_key(self) -> Optional[tuple[str, int]]:
        """
        Разделяет название тикета на название отдела и номер тикета

        пример: "CLOUD-12345" -> ("CLOUD", 12345)
        """
        key = self.key
        if key is None:
            return None

        temp = key.split('-')
        try:
            queue, num = temp
            num = int(num)
        except ValueError:
            queue, num = key, 0
        return queue, num

    @property
    def priority(self):
        if self._priority is None:
            return sys.float_info.max
        else:
            return self._priority

    @priority.setter
    def priority(self, value):
        self._priority = value

    @property
    def raw_priority(self):
        if self._priority_field is None:
            return None
        return getattr(self, self._priority_field)

    @classmethod
    def get_or_load(cls: Type[T],
                    issue_key: str,
                    st_client: StartrekClient) -> T:
        """
        Gets issue from cache and returns it

        If issue_id not in cache issue from startrek, ads it to cache and returns it
        """
        if issue_key not in cls.cache:
            issue_data = st_client.find(issue_key, cls._fields)
            cls.cache[issue_key] = cls(issue_data)

        return cls.cache[issue_key]

    def get_epic(self: T, st_client: StartrekClient) -> Optional[T]:
        try:
            if self.epic_key is None:
                return None
            return self.get_or_load(self.epic_key, st_client)
        except HTTPError as e:
            if e.response.status_code == requests.codes.forbidden:
                print(f'Cannot load {self.epic_key} (epic of {self.key})\n')
                return None
            raise e

    def get_parent(self: T, st_client: StartrekClient) -> Optional[T]:
        try:
            if self.parent_key is None:
                return None
            return self.get_or_load(self.parent_key, st_client)
        except HTTPError as e:
            if e.response.status_code == requests.codes.forbidden:
                print(f'Cannot load {self.parent_key} (parent of {self.key})\n')
                return None
            raise e

    def get_group_issue(self: T, st_client: StartrekClient, path=None) -> T:
        """
        Возвращает top-level задачу

        Args:
            st_client: клиент трекера

        Returns:
            top-level issue
        """
        issue = self
        while True:
            if path is not None:
                path.append(issue)
            if (epic := issue.get_epic(st_client)) is not None:
                issue = epic
            elif (parent := issue.get_parent(st_client)) is not None:
                issue = parent
            else:
                return issue

    def collect_issues(self, nodes_by_id: dict[int, Node], st_client: StartrekClient):
        """заполняет links и sub_issues"""
        if self.sub_issues_filled:
            return

        self.sub_issues_filled = True
        for sub_issue_node in get_or_load_issue_node(nodes_by_id, self, st_client.links).children:
            try:
                sub_issue = self.get_or_load(sub_issue_node.id, st_client)
            except:
                print(f'Cannot load {sub_issue_node.key}')
                continue
            self.sub_issues[sub_issue.key] = sub_issue
            sub_issue.collect_issues(nodes_by_id, st_client)

    def collect_categories(self, category, path, result):
        """Записывает в result все пути (в виде list) к категориям, к которым подошла issue по condition"""
        path = path + [category['name']]

        if 'condition' in category:
            if eval(category['condition'], {}, {'issue': self}):
                result.append(path)

        if 'items' in category:
            for sub_category in category['items']:
                self.collect_categories(sub_category, path, result)

    def get_category(self, system_categories, categories):
        """Находит категории (в виде list - пути к ним в config.yaml), к которым подходит issue по condition
        в config.yaml возвращает первую из них"""
        result = []

        for category in categories:
            self.collect_categories(category, [], result)

        if len(result) == 0:
            # если категория не найдена, попадает в системную категорию no_category
            return system_categories.get('no_category')

        if len(result) > 1:
            print(f'More than one category for the issue {self.key} {str(result)}, the first one was chosen')

        return result[0]

    def sort_key(self, top_level):
        # подзадачи сортируем по названию тикета
        if not top_level:
            return self.parse_key()

        return [self.importance if self.importance is not None else sys.float_info.max,
                0 if self.issue_data['status']['key'] not in ('new', 'backlog') else 1,
                self.parse_key()]

    def filter_sub_issues(self, func: Callable[['Issue'], bool]):
        for sub_issue in self.sub_issues.values():
            sub_issue.filter_sub_issues(func)
        self.sub_issues = {k: v for k, v in filter(lambda kv: func(kv[1]), self.sub_issues.items())}

    @staticmethod
    def get_comments(issue_id: str, st_client):
        try:
            return st_client.get_comments(issue_id)
        except HTTPError as e:
            if e.response.status_code == requests.codes.forbidden:
                print(f'Cannot load comments of issue{issue_id}\n')
                return []
            raise e

    @staticmethod
    def get_html_comment(issue_id: str, comment_id: int, st_client) -> Optional[str]:
        # возвращает безопасный html код
        try:
            return st_client.get_html_comment(issue_id, comment_id)
        except HTTPError as e:
            if e.response.status_code == requests.codes.forbidden:
                print(f'Cannot load comment {comment_id} of issue{issue_id}\n')
                return None
            raise e


class TaskTree:
    def __init__(self, *, collapsed=None,
                 visible=None,
                 chart=None,
                 reverse_priority=False,
                 collapse_equal_priority=False):
        # название подкатегории -> TaskTree
        self.items: OrderedDict[str, TaskTree] = OrderedDict()
        self.issues: list[Issue] = []
        """задачи в текущей категории"""

        # заполняются в _categorize_issue
        self.collapsed = collapsed
        """указывает, будет ли скрыта или развернута ветка в отчете, по умолчанию False"""
        self.visible = visible
        self.chart = chart
        self.reverse_priority = reverse_priority
        self.collapse_equal_priority = collapse_equal_priority

    # добавляет issue в task_tree по пути, заполняем время в task_tree
    def categorize_issue(self, issue: Issue, categories):
        last = self

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

        last.issues.append(issue)

    def sort(self, category, is_category):
        # сортируем task.items
        # значение для сортировки - номер первого вхождения ключа в category['items']
        # т. е. порядок как в config.yaml
        self.items = OrderedDict(sorted(self.items.items(),
                                        key=lambda kv: next(i for i, v in enumerate(category['items'])
                                                            if v['name'] == kv[0])))
        for key, value in self.items.items():
            # вместо category подставляем подкатегорию с именем key
            value.sort(next(iter(filter(lambda x: x['name'] == key, category['items']))), True)

        # сортируем task.issues
        self.issues = sorted(self.issues, key=lambda x: x.sort_key(is_category))
        if is_category:
            self.prioritize_by_field()

    def prioritize_by_field(self):
        # по ключу задачи ставит (номер ее importance в списке задач по возрастанию importance)
        collapse = 0
        previous_priority = None

        for i, issue in enumerate(sorted(filter(lambda x: x.raw_priority is not None, self.issues),
                                         key=lambda x: x.raw_priority,
                                         reverse=self.reverse_priority), start=1):
            if self.collapse_equal_priority and issue.raw_priority == previous_priority:
                collapse += 1
            else:
                previous_priority = issue.raw_priority
            issue.priority = i - collapse

    def filter_issues(self, func: Callable[[Issue], bool]):
        for task_tree in self.items.values():
            task_tree.filter_issues(func)
        self.issues = [*filter(func, self.issues)]
        for issue in self.issues:
            issue.filter_sub_issues(func)


class MyABC(abc.ABC):
    def __new__(cls, *args, **kwargs):
        inst = super(MyABC, cls).__new__(cls)
        inst.__init__(*args, **kwargs)
        not_implemented = [k for k, v in inst.__dict__.items() if v is NotImplemented]
        if len(not_implemented) > 0:
            raise TypeError(f"Can't instantiate {cls} with abstract members: {not_implemented}")
        return inst


class Report(MyABC):
    template_name: str = NotImplemented
    mds_path: str = NotImplemented
    'dot-separated string, describing yaml-path to the mds_path'

    def __init__(self, config: Config, name):
        self._config = config
        self._yaml = config.evaluate({'report': name})

        self._team = self._yaml['timesheet']['team']
        self._board_id = self._yaml['timesheet']['board_id']

        self._startrek_url = self._yaml['startrek']['ui']['url']
        self._staff_url = self._yaml['staff']['ui']['url']
        self._center_url = self._yaml['center']['ui']['url']

        self._day_start = datetime.time.fromisoformat(
            self._yaml['worklog']['day_start'])
        self._day_length = parse_duration(
            self._yaml['worklog'].get('day_length', '8h'))
        self._timezone = pytz.timezone(self._yaml['worklog']['timezone'])
        self._rendered = None

        self._render_params: dict = NotImplemented

    @classmethod
    def _delete_not_existent_categories(cls, category):
        if 'exists' in category:
            if not category['exists']:
                return False

        if 'items' in category:
            for item in list(category['items']):
                if not cls._delete_not_existent_categories(item):
                    category['items'].remove(item)

        return True

    @property
    def render_params(self):
        return self._render_params

    @classmethod
    def _fill_system_categories(cls, category, path, system_categories):
        """
        находит system в категориях и заполняет словарь system_categories: name -> путь в виде list


        *Пример:*
        system_categories = {'no_category': ['Эксплуатация', 'Поддержка внутренних пользователей'],
        'absence': ['Отсутствия']}

        :param category:
        :param path:
        :param system_categories:
        :return:
        """
        if 'system' in category:
            system_categories[category['system']] = path

        if 'items' in category:
            for item in category['items']:
                cls._fill_system_categories(item, path + [item['name']], system_categories)

    def _get_worklog_issues(self, from_date, to_date, fields):
        worklog_issues = []
        for user in self._team:
            for wl in self._config.startrek.worklog_by_person(user, from_date, to_date):
                worklog_issues.append(self._config.startrek.find(wl['issue']['id'], fields))
        return worklog_issues


class Util:
    def __init__(self, report_cls, config: Config, prepared, auto_beautify=False):
        self._rendered = None
        self.report_cls = report_cls
        self.config = config
        self.prepared: dict = prepared
        self.auto_beautify = auto_beautify

    @staticmethod
    def validate_class(report_cls):
        if not issubclass(report_cls, Report):
            raise ValueError('report_cls must Report subclass')

    def render(self, template_name: str = None):
        if template_name is None:
            template_name = self.report_cls.template_name
        styled_print('[-] Rendering report')
        self._rendered = self.config.jinja.get_template(template_name).render(self.prepared)
        if self.auto_beautify:
            self.beautify()
        styled_reprint('[+] Rendering report')

    @property
    def rendered(self) -> str:
        if self._rendered is None:
            self.render()
        return self._rendered

    def save(self, path):
        """Save report to the file"""
        styled_print(f'[-] Saving report to the {path}')
        with open(path, 'w', encoding='utf-8') as file:
            file.write(self.rendered)
        styled_reprint(f'[+] Saving report to the {path}')

    def save_prepared(self, path):
        styled_print(f'[-] Saving raw-report to the {path}')
        with open(path, 'wb') as file:
            pickle.dump({'prepared': self.prepared, 'metadata': {'class': self.report_cls}}, file)
        styled_reprint(f'[+] Saving raw-report to the {path}')

    def beautify(self):
        soup = BeautifulSoup(self.rendered, 'html.parser')
        self._rendered = soup.prettify(encoding='utf8').decode('utf8')

    def upload(self, path: str = '', release_date: datetime.date = None, date_format: str = "%Y-%m-%d"):
        upload_path = self.config._yaml
        for k in self.report_cls.mds_path.split('.'):
            upload_path = upload_path[k]
        if path != '':
            if path.startswith('/'):
                upload_path += path
            else:
                upload_path += f'/{path}'
        if release_date is not None:
            upload_path += f'/{release_date.strftime(date_format)}'
        styled_print(f'[-] Uploading report to the {upload_path}')
        self.config.website.upload(upload_path, self.rendered)
        styled_reprint(f'[+] Uploading report to the {upload_path}')


class UtilBuilder:
    def __init__(self, config: Config):
        self._prepared_report = None
        self._report_cls = None
        self._config = config

    def set_report_cls(self, report_cls):
        Util.validate_class(report_cls)
        self._report_cls = report_cls

    def prepare_from_file(self, path):
        with open(path, 'rb') as file:
            data = pickle.load(file, encoding='utf8')
            self._prepared_report = data['prepared']
            self.set_report_cls(data['metadata']['class'])

    def prepare_from_web(self, *args, **kwargs):
        report = self._report_cls(self._config)
        report.prepare(*args, **kwargs)
        self._prepared_report = report.render_params

    def build(self):
        return Util(self._report_cls, self._config, self._prepared_report)
