# -*- coding: utf-8 -*-

from ids.registry import registry


rep_issues = registry.get_repository('startrek', 'issues',
                server='https://st-api.test.yandex-team.ru',
                user_agent='myservice',
                oauth2_access_token='cce8de3df9594e008681106e39fefc80',
            )

issue = rep_issues.get_one(lookup={'key': 'WIKI-100'})
print '#{0} found by key'.format(issue['key'])

issues = rep_issues.get(lookup={'key': ['STARTREK-13','STARTREK-14']})
print '#{0} found'.format(len(issues))


rep_queues = registry.get_repository('startrek', 'queues',
                user_agent='myservice',
                server='https://st-api.test.yandex-team.ru',
                oauth2_access_token='cce8de3df9594e008681106e39fefc80',
            )

queue = rep_queues.get_one({'key':'TEST'})

parent_issue = rep_issues.get_one(lookup={'key':'TEST-100'})

new_issue_fields = {
            'queue': {'id': queue["id"]},
            'summary': 'New test-issue',
            "parentIssue" : {"id":parent_issue["id"]}}

sub_issue = rep_issues.create(fields = new_issue_fields)

print "created #{0} #{1}".format(sub_issue["id"], sub_issue["key"])

