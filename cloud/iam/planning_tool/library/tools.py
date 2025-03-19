# -*- coding: utf-8 -*-

class PlanningTools(object):
    def __init__(self, config):
        self._config = config

        self._fields = ["id", "key", "status", "summary", "epic", "tags", "type", "parent", "queue"]

        self._components = self._config.yaml['timesheet']['components']
        self._not_in_progress_statuses = config.yaml['timesheet']['not_in_progress_statuses']

    @staticmethod
    def _group_issues(issues):
        result = dict()
        for issue in issues:
            result[issue['key']] = issue

        return result

    @classmethod
    def _print_with_children(cls, issue, indent):
        indent_str = '\t' * indent
        print(f"{indent_str}{issue['key']} {issue['summary']}")
        if 'children' in issue:
            for child in sorted(issue['children']):
                cls._print_with_children(issue['children'][child], indent + 1)

    def check_issues(self):
        temp = ','.join(map(lambda x: f'"{x}"', self._components))
        query = f'Status: !Closed AND Components: {temp}'
        issues = self._group_issues(self._config.startrek.search(query, self._fields))

        epics = dict()
        invalid_epic_issues = dict()
        no_epic_issues = dict()
        invalid_status_issues = dict()

        for key in issues:
            issue = issues[key]
            if issue['type']['key'] == 'epic':
                epics[key] = issue

        for key in list(issues.keys()):
            issue = issues[key]

            if 'tags' in issue and 'duty' in issue['tags']:
                continue

            if 'tags' in issue and 'no_tag' in issue['tags']:
                continue

            if issue['type']['key'] == 'epic':
                continue

            if (issue['type']['key'] == 'bug' or
                    issue['type']['key'] == 'release' or
                    issue['queue']['key'] == 'CLOUDINC' or
                    issue['queue']['key'] == 'CLOUDDUTY') and \
                    'parent' not in issue and 'epic' not in issue:
                continue

            in_progress = issue['status']['display'] not in self._not_in_progress_statuses

            while 'parent' in issue and 'epic' not in issue:
                parent_issue = issues.get(issue['parent']['key'])
                if not parent_issue:
                    parent_issue = self._config.startrek.find(issue['parent']['id'])
                    issues[parent_issue['key']] = parent_issue

                parent_in_progress = parent_issue['status']['display'] not in self._not_in_progress_statuses
                if in_progress and not parent_in_progress:
                    invalid_status_issues[key] = issue

                parent_issue.setdefault('children', dict())[issue['key']] = issue
                issue = parent_issue

            if issue['type']['key'] != 'epic':
                if issue.get('epic') is not None:
                    epic_key = issue['epic']['key']
                    if 'no_epic' in issue.get('tags', []):
                        pass
                    elif epic_key not in epics:
                        invalid_epic_issues[key] = issues[key]
                    else:
                        epic = epics[epic_key]
                        epic.setdefault('children', dict())[issue['key']] = issue

                        epic_in_progress = epic['status']['display'] not in self._not_in_progress_statuses
                        if in_progress and not epic_in_progress:
                            invalid_status_issues[key] = issue
                elif 'no_epic' not in issue.get('tags', []):
                    no_epic_issues[key] = issues[key]
            elif 'no_epic' not in issue.get('tags', []):
                no_epic_issues[key] = issues[key]

        print(f'Issues {str(len(issues))}')
        print(f'Epics {str(len(epics))}')

        for epic_key in sorted(epics):
            epic = epics[epic_key]
            self._print_with_children(epic, 0)

        print()
        print(f'Invalid epic {str(len(invalid_epic_issues))}')

        for key in sorted(invalid_epic_issues):
            print(f"{key} {invalid_epic_issues[key]['summary']}")

        print()
        print(f'No epic {str(len(no_epic_issues))}')

        for key in sorted(no_epic_issues):
            print(f"{key} {no_epic_issues[key]['summary']}")

        print()
        print(f'Invalid status {str(len(invalid_status_issues))}')

        for key in sorted(invalid_status_issues):
            print(f"{key} {invalid_status_issues[key]['summary']}")
