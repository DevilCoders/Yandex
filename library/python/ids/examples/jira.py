# -*- coding: utf-8 -*-

from ids.registry import registry


tickets = registry.get_repository('jira', 'tickets',
                server='https://jira.test.tools.yandex-team.ru',
                oauth2_access_token='5431b13a8cca4cc5b8b4cce46929d3aa',
                fields_mapping={'planner_project_id': 'customfield_11830'},
            )

ticket = tickets.get_one(lookup={'key': 'PLAN-1'})
print '#{0} found by key'.format(ticket['key'])

t = tickets.get_one(lookup={'planner_project_id': 666})
print '#{0} (with custom field {1}) keys:\n{2}'\
            .format(t['key'], int(t['planner_project_id']), ', '.join(t.keys()))
tickets.update(t, fields={'planner_project_id': 123})
print 'new planner_project_id: {0}'.format(int(t['planner_project_id']))

reopened_tickets = tickets.get(lookup={'status__exact': 'reopened'})
print 'reopened count: {0}'.format(len(reopened_tickets))

ts = tickets.getiter(lookup={'key__in': ['PLAN-666', 'PLAN-123']})
print map(lambda x: x['key'], ts)

new_ticket = tickets.create(fields={
                                'project': {'key': 'PLAN'},
                                'summary': 'New issue from ids-jira',
                            })
print 'new ({2}): {0} - {1}'.format(new_ticket['key'],
                                    new_ticket['summary'],
                                    new_ticket['issuetype']['name'])

tickets.update(new_ticket, fields={'summary': new_ticket['summary'] + '_suffix'})
print 'updated: {0} - {1}'.format(new_ticket['key'], new_ticket['summary'])
