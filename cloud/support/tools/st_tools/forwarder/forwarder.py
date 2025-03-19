from helpers import *

client = SupportST(useragent="cloud-marketplace",
                  base_url="https://st-api.yandex-team.ru",
                  token=get_token())

request = 'Resolution: empty() Queue: CLOUDSUPPORT Status: open, inProgress Components: into_print Tags: !"answered_by_service"'

issues = client.issues.find(request)

for issue in issues:

    for c in reversed(list(issue.comments.get_all(expand="attachments"))):


        if c.createdBy.login not in ['robot-yc-support-api',
                                     'robot-ycloudsupport'] and c.text != '' and c.type != 'outgoing' \
                and len(c.summonees) == 0 and parse_reply_tags(c.text) != '':

            client.post_and_change_status(issue, c.text, add_tags=['answered_by_service'],
                                          attachments=[i for i in c.attachments])
            break



def main():
    pass 
