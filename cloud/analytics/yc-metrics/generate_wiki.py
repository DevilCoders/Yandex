import os
import requests
import re
from glob import glob
import json

headers = {'Authorization': "OAuth "+os.environ['WIKI_TOKEN']}
params = {'format':'json'}
url = 'https://wiki-api.yandex-team.ru/_api/frontend/cloud-bizdev/analitika-oblaka/kljuchevye-metriki-oblaka/'


regexp = r'(___DOC___)([\s\S]*?)(___DOC___)'
curr_path = '/'.join(os.path.realpath(__file__).split('/')[:-1]) + '/metrics'


def get_folders_list(curr_path):
    folder_list = []
    for folder in os.listdir(curr_path):
        if os.path.isdir(os.path.join(curr_path, folder)):
            folder_list.append(folder)
    return(folder_list)

def get_file_list(folder,curr_path):
    return glob(curr_path+"/"+folder+"/*")

def get_wiki(files_list,wiki_list):
    for metric in files_list:
        if os.path.isfile(metric):
            f = open(metric)
            metric_descrition = re.search(regexp,f.read())
            wiki_list.append(metric_descrition[2]+'\n'+'\n'+'\n')
            f.close()
        else:
            continue


wiki_list = []
for folder in get_folders_list(curr_path):
    wiki_list.append('\n'+'\n'+'\n'+'\n'+'=='+folder[0].upper() + folder[1:]+'\n'+'\n')
    get_wiki(get_file_list(folder,curr_path),wiki_list)

body = ''.join(wiki_list)

data = {"title":"2. Ключевые метрики Облака", "body": body}

requests.post(url, headers = headers, params = params, data = data)
#
# with open('output.json', 'w') as f:
#         json.dump({
#             "status": 'success'
#         }, f)
