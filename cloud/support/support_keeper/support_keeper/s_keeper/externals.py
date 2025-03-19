import aiohttp
import requests
import os


TOKEN = os.environ.get('S_TOKEN', 'YouForgotYourToken')
async def download_persons():
    def make_params(min_id, department_url):
        return {
            '_nopage': 1,
            '_sort': 'id',
            '_query': f'id > {min_id} and (group.url == "{department_url}" or group.ancestors.url == "{department_url}")',
            '_fields': 'id,person.login',
            'group.type': 'department',
            'person.official.is_dismissed': 'false',
        }
    department_url = 'yandex_mnt_cloud_support' #'yandex_infra_data'
    url = 'https://staff-api.yandex-team.ru/v3/groupmembership'
    token = TOKEN
    headers = {'Authorization': f'OAuth {token}'}
    persons = []
    async with aiohttp.ClientSession(headers=headers) as session:
        last_id = -1
        while True:
            params = make_params(last_id, department_url)
            async with session.get(url=url, params=params) as response:
                response_data = await response.json()
                if not response_data['result']:
                    break
                last_id = response_data['result'][-1]['id']
                persons += response_data['result']
    return persons


def get_components():
    url = 'https://st-api.yandex-team.ru/v2/queues/CLOUDSUPPORT/components'
    token = TOKEN
    headers = {'Authorization': f'OAuth {token}'}
    components = []
    r = requests.get(url=url, headers=headers)
    res = r.json()
    return res

def get_ranks():
    ranks =[
       "Шеф", "Зам", "Коллега", "Курсант", "Асессор", "Помогатор", "Всё сложно"
    ]
    return ranks
