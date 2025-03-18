import os
import requests

SOLOMON_API_URL = 'http://solomon.yandex.net/api/v2/projects/Antiadblock/sensors/data'
SOLOMON_TOKEN = os.getenv('SOLOMON_TOKEN')


def get_data_from_solomon(program, downsampling, start_date, end_date, **kwargs):
    data = {"program": program, "downsampling": downsampling, "from": start_date, "to": end_date, "useNewFormat": False}
    if kwargs:
        data.update(kwargs)
    response = requests.post(SOLOMON_API_URL,
                             headers={'Authorization': 'OAuth {}'.format(SOLOMON_TOKEN), 'Accept': 'application/json',
                                      'Content-Type': 'application/json;charset=UTF-8'},
                             json=data)
    assert response.status_code == 200, \
        'Cannot get data from Solomon.\n Reason: {}'.format(response.json().get('message', 'No reason'))
    return response.json().get('vector')
