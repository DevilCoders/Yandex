# -*- coding: utf-8 -*-

import isodate

from cloud.iam.planning_tool.library.report import Report, Issue, TaskTree
from cloud.iam.planning_tool.library.utils import styled_print, styled_reprint


class TaxIssue(Issue):
    def __init__(self, issue_data: dict):
        super().__init__(issue_data)
        self.sub_issues: dict[int, TaxIssue] = {}
        self.components = issue_data.get('components', [])

    def has_pr(self, from_date, to_date):
        for link in self.remote_links:
            if link['object']['application']['id'] in ['ru.yandex.arcanum', 'ru.yandex.bitbucket']:
                created = isodate.parse_datetime(link['createdAt']).date()
                if from_date <= created < to_date:
                    return True

        return False

    def collect_categories(self, category, path, result, system_categories, from_date, to_date):
        """Записывает в result все пути (в виде list) к категориям, к которым подошла issue по condition"""
        path = path + [category['name']]

        if 'condition' in category:
            if eval(category['condition'], {}, {'issue': self}):
                if not category.get('need_prs', False) or self.has_pr(from_date, to_date):
                    result.insert(0, path)
                else:
                    result.append(system_categories.get('no_pr'))

        if 'items' in category:
            for sub_category in category['items']:
                self.collect_categories(sub_category, path, result, system_categories, from_date, to_date)

    def get_category(self, system_categories, categories, from_date, to_date):
        result = []

        for category in categories:
            self.collect_categories(category, [], result, system_categories, from_date, to_date)

        if len(result) == 0:
            # если категория не найдена, попадает в системную категорию no_category
            return system_categories.get('no_category')

        if len(result) > 1:
            print(f'More than one category for the issue {self.key} {result}, the first one was chosen')

        return result[0]


class TaxTaskTree(TaskTree):
    # добавляет issue.top_level_issue в task_tree по пути категории
    def categorize_issue(self, issue, categories):
        last = self

        category = {'items': categories}
        # перебираем путь к категории
        for category_key in issue.category:
            # категория в category['items'], равная category_key
            category = next(iter(filter(lambda x: x['name'] == category_key, category['items'])))
            if category_key not in last.items:
                last.items[category_key] = type(self)(collapsed=category.get('collapsed', False),
                                                      visible=category.get('visible', True),
                                                      chart=category.get('chart', True))

            last = last.items[category_key]

        for i in last.issues:
            if i.key == issue.top_level_issue.key:
                return

        last.issues.append(issue.top_level_issue)


class TaxReport(Report):
    template_name = 'tax.wiki'
    mds_path = 'reports.tax.mds.path'

    def __init__(self, config):
        super(TaxReport, self).__init__(config, 'tax')
        self._filter = self._yaml['tax']['filter']
        self._categories = self._yaml['tax']['categories']
        self._fields = list({'id',
                             'key',
                             'status',
                             'summary',
                             'epic',
                             'parent',
                             'components'})

        self._render_params = {'startrek_url': self._startrek_url}

    def _task_query(self, from_date, to_date):
        query = f'({self._filter})'

        if from_date is not None:
            query += f' AND Updated: >= {from_date.strftime("%Y-%m-%d")}'

        if to_date is not None:
            query += f' AND Updated: < {to_date.strftime("%Y-%m-%d")}'

        return query

    def upload(self, path, title, rendered):
        self._config.wiki.upload(path, title, rendered)

    def prepare(self, from_date, to_date):
        print()
        issues = self._config.startrek.search(self._task_query(from_date, to_date), self._fields)
        issues = [TaxIssue(issue_data) for issue_data in issues]
        styled_print('[-] Collecting group issues')
        for issue in issues:
            group_issue = issue.get_group_issue(self._config.startrek)
            issue.top_level_issue = group_issue
            styled_reprint('[-] Collecting group issues')
        styled_reprint('[+] Collecting group issues')
        self._delete_not_existent_categories({'items': self._categories})
        system_categories = {}
        self._fill_system_categories({'items': self._categories}, [], system_categories)
        task_tree = TaxTaskTree()
        for issue in issues:
            if issue.remote_links is None:
                issue.remote_links = self._config.startrek.remote_links(issue.key)
            issue.category = issue.get_category(system_categories, self._categories, from_date, to_date)

            task_tree.categorize_issue(issue, self._categories)
        task_tree.sort({'items': self._categories}, False)
        self._render_params['task_tree'] = task_tree
