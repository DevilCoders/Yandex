# coding=utf-8
from startrek_client import Startrek
from  startrek_client import exceptions as st_ex
import csv, datetime
from helpers import get_token
client = Startrek(useragent="cloud-marketplace",
                  base_url="https://st-api.yandex-team.ru",
                  token=get_token())
supers = ['sinkvey', 'nooss', 'the-nans','apereshein' ]


month = input('please input month:')
d1 = datetime.date(year=datetime.date.today().year, month=int(month), day=1)
d2 = datetime.date(year=datetime.date.today().year, month=int(month)+1, day=1)
csvfile = open(f'{d1.isoformat()[0:-3]}.csv', 'w', newline='')
writer = csv.writer(csvfile)
writer.writerow(['ticket', 'supervisor', 'support_changed', 'timestamp_changed', 'timestamp_supervised', 'likes_from_super', 'comments_from_super', 'outgoing_comments_from_super', 'link_to_history'])

#for supr in supers:   
req_supervised=f'Queue: CLOUDSUPPORT,CLOUDLINETWO Supervisor_required: changed( to: Нет  date: >={d1} ) Supervisor_required: changed( to: Нет date: <{d2} ) "Sort by": Updated ASC' # by: {supr}

issues = client.issues.find(req_supervised)
for issue in issues:
    supervisor = ""
    support_changed = ""
    timestamp_changed = ""
    timestamp_supervised = ""
    likes_from_super = 0
    comments_from_super = 0
    outgoing_comments_from_super = 0

    for log in issue.changelog:
        if (datetime.datetime.fromisoformat(log.updatedAt[0:-5]).date() + datetime.timedelta(hours=3)) >= d2 or  (datetime.datetime.fromisoformat(log.updatedAt[0:-5]).date() + datetime.timedelta(hours=3)) < d1:
            continue
        for field in log.fields: #== '62c7d75a9564827ce59db5a7':

            f = field['field'].name or "" 
            ffrom = field['from'] or ""
            fto = field['to'] or ""
            
            if f == 'supervisor_required' and fto == "Да": #and ffrom == ""
                support_changed = log.updatedBy.login
                timestamp_changed = datetime.datetime.fromisoformat(log.updatedAt[0:-5])
                print(f'{issue.key} - {log.updatedBy.login} changed from {ffrom} to {fto} at {log.updatedAt}')
            if f == 'supervisor_required' and fto == "Нет" and (log.updatedBy.login in supers): #and ffrom == "Да"
                supervisor = log.updatedBy.login
                timestamp_supervised = datetime.datetime.fromisoformat(log.updatedAt[0:-5])
                print(f'{issue.key} - {log.updatedBy.login} changed from {ffrom} to {fto} at {log.updatedAt}')
                time1 = timestamp_supervised - datetime.timedelta(hours=2)
                time2 = timestamp_supervised + datetime.timedelta(hours=2)

                for log2 in issue.changelog:
                    if log2.updatedBy.login == supervisor:
                        if log2.comments is not None and log2.comments.get('updated'):
                            if datetime.datetime.fromisoformat(log2.updatedAt[0:-5]) >= time1 and datetime.datetime.fromisoformat(log2.updatedAt[0:-5]) <= time2:
                                try:
                                    if log2.comments['updated'][0].get('addedReaction'):
                                        likes_from_super += 1
                                except st_ex.NotFound:
                                    pass
                        if log2.comments is not None and log2.comments.get('added'):
                            if datetime.datetime.fromisoformat(log2.updatedAt[0:-5]) >= time1 and datetime.datetime.fromisoformat(log2.updatedAt[0:-5]) <= time2:
                                #print(f'{log.id} - {log2.id}')
                                try:
                                    if log2.comments['added'][0].createdBy.login == supervisor:
                                        if log2.comments['added'][0].type == 'outgoing':
                                            outgoing_comments_from_super += 1
                                        else:
                                            comments_from_super += 1
                                except st_ex.NotFound:
                                    pass
              
                if timestamp_changed != "":
                    timestamp_changed = str(timestamp_changed+ datetime.timedelta(hours=3))
                timestamp_supervised = str(timestamp_supervised+ datetime.timedelta(hours=3))
                writer.writerow([issue.key, supervisor, support_changed, timestamp_changed[0:-7], timestamp_supervised[0:-7], likes_from_super, comments_from_super, outgoing_comments_from_super, f'https://st.yandex-team.ru/{issue.key}/history#{log.id}'])
                supervisor = ""
                support_changed = ""
                timestamp_changed = ""
                timestamp_supervised = ""
                likes_from_super = 0
                comments_from_super = 0
                outgoing_comments_from_super = 0


def main():
    pass
