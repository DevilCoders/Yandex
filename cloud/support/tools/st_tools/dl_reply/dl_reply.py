from startrek_client import exceptions as st_exceptions
from startrek_client import Startrek
from helpers import get_token
from time import sleep

issue_counter = 0
issue_list = []
not_fucked_list = []

comment_podcats = """ 
Добрый день!

Заполните, пожалуйста, форму https://forms.yandex.ru/surveys/10022998.21081a59bf8accc3300d8e031b1d9442deca87e6/ . 
Она специально создана для решения проблем пользователей DataLens / Подкастеров Яндекс.Музыки

"""


def has_outgoing(comments):
    for c in comments.get_all():
        if c.type == 'outgoing':
            return True
    return False


client = Startrek(useragent="cloud-marketplace",
                  base_url="https://st-api.yandex-team.ru",
                  token=get_token())

issues = client.issues.find(
    'Queue: CLOUDSUPPORT status: Open Components: datalens pay: free, unknown, empty() "Sort By": SLA ASC')

ana_comment = 1

for issue in issues:
    if len(issue.links.get_all()) == 0:
        if has_outgoing(issue.comments):
            break
        issue_list.append(issue.key)

print('\n'.join(issue_list))

for issue_key in issue_list:
    n_issue = client.issues[issue_key]

    if 'подкаст' in n_issue.description or 'ch_ya_music_podcast_stats' in list(n_issue.commetns)[1].text:
        comment = comment_podcats

        n_issue.comments.create(text=comment,
                                type="outgoing",
                                )
        print("{} commented".format(issue_key))
        try:
            sleep(1)
            if n_issue.transitions.get("partiallyCompleted"):
                n_issue.transitions["partiallyCompleted"].execute()
                sleep(1)
                tags = n_issue.tags or []
                n_issue = client.issues[issue_key]
                n_issue.update(tags=tags + ["bot_rassylka", "datalens_free"])
        except st_exceptions.NotFound:
            print("{} can not move to `partiallyCompleted`".format(issue_key))
        else:
            print("{} moved to `partiallyCompleted`".format(issue_key))


def main():
    pass
