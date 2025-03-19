from startrek_client import Startrek
from helpers import get_token
client = Startrek(useragent="cloud-marketplace",
                  base_url="https://st-api.yandex-team.ru",
                  token=get_token())

req_components_not_set='Resolution: empty() Queue: CLOUDSUPPORT Status: open Components: empty()'

issues = client.issues.find(req_components_not_set)
for issue in issues:
    for comment in issue.comments:
        if 'Дополнительная информация' in comment.text and 'datalens' in comment.text:
            components = issue.components
            issue.update(components=components + ["datalens"])
            print(issue.key)


def main():
    pass
