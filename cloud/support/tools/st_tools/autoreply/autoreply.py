from helpers import *


comment ="""
Мы получили ваше обращение и обязательно на него ответим. По техническим причинам сегодня вечером наш ответ может задержаться. 

Мы восстановим нормальную работу наших систем завтра утром. 

Спасибо, что остаетесь с нами!
"""

request = 'Pay: business, standard Resolution: empty() Queue: CLOUDSUPPORT Status: open, inProgress Tags: !"ergo_bibamus" Updated: >= today()+15h'


client = SupportST(useragent="cloud-marketplace",
                   base_url="https://st-api.yandex-team.ru",
                   token=get_token())

issues = client.issues.find(request)

for issue in issues:

    if last_human_comment(issue.comments):
        if 'ergo_bibamus' not in issue.tags:
            client.post_and_change_status(issue, comment, ['ergo_bibamus'], "waitForAnswer" )

