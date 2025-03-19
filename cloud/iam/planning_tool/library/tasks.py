# -*- coding: utf-8 -*-

from cloud.iam.planning_tool.library.report import Report, Issue
from cloud.iam.planning_tool.library.report import TaskTree
from cloud.iam.planning_tool.library.utils import Node, styled_print, styled_reprint


class TaskReport(Report):
    template_name = 'tasks.html'
    mds_path = 'reports..mds.path'

    def __init__(self, config):
        super(TaskReport, self).__init__(config, 'tasks')
        self._categories = self._yaml['timesheet']['categories']
        self._components = self._yaml['timesheet']['components']
        Issue.update_fields({'id',
                             'key',
                             'status',
                             'summary',
                             'epic',
                             'parent',
                             'links',
                             'tags',
                             'importance'}.union(self._yaml['reports']['tasks']['additional_fields']))
        self._render_params = {'day_start': self._day_start,
                               'day_length': self._day_length,
                               'timezone': self._timezone,
                               'startrek_url': self._startrek_url}

    def _task_query(self, show_closed, created_from_date):
        temp = ' OR '.join(map(lambda component: f'Components: "{component}"', self._components))
        query = f'({temp})'

        if not show_closed:
            query += ' AND Resolution: empty()'

        if created_from_date is not None:
            query += f' AND Created: >= {created_from_date.strftime("%Y-%m-%d")}'

        return query

    def prepare(self, created_from_date, show_closed):
        print()
        issues = self._config.startrek.search(self._task_query(show_closed, created_from_date), Issue.get_fields())
        Issue.create_many(issues)
        # Add all parent issues and every their child to calculate the total time spent for every issue at the top level
        styled_print('[-] Collecting related issues')
        group_issues_by_key: dict[str, Issue] = {}  # top_level_issue_key -> top_level_issue
        nodes_by_id: dict[int, Node] = {}
        for issue_id, issue in [*Issue.cache.items()]:
            group_issue = issue.get_group_issue(self._config.startrek)
            issue.top_level_issue = group_issue
            group_issues_by_key[group_issue.key] = group_issue
            group_issue.collect_issues(nodes_by_id, self._config.startrek)
            styled_reprint(f'[-] Collecting related issues ({len(Issue.cache)} issues collected)')
        styled_reprint(f'[+] Collecting related issues ({len(Issue.cache)} issues collected)')
        self._delete_not_existent_categories({'items': self._categories})
        system_categories = {}
        self._fill_system_categories({'items': self._categories}, [], system_categories)
        task_tree = TaskTree()
        for key, issue in group_issues_by_key.items():
            issue.category = issue.get_category(system_categories, self._categories)
            if issue.category is not None:
                task_tree.categorize_issue(issue, self._categories)
        task_tree.sort({'items': self._categories}, False)
        self._render_params['task_tree'] = task_tree
