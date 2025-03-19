import requests
import json
import logging
import time
from uuid import uuid4


logging.basicConfig()
logging.getLogger().setLevel(logging.DEBUG)
requests_log = logging.getLogger("requests.packages.urllib3")
requests_log.setLevel(logging.DEBUG)
requests_log.propagate = True


def main():
    job_processor_url = input('job processor url: ')
    with open('example_start_request_0.json', 'r') as f:
        job_json = json.load(f)

#    while True:
    instance_id = str(uuid4())
    print('\n\n')
    print('instance_id: ', instance_id)
    response = requests.put(
        f'{job_processor_url}/v2/call/run/{instance_id}',
        data=json.dumps(job_json),
        headers={'Content-Type': 'application/json'}
    )

    print('\n')
    print('response', response.status_code)
    print(response.content)
    if response.status_code == 200:
        result = response.json()
        print(result)

        ticket = result['ticket']
        print(f'ticket: {ticket}')

        while input('get job status? ') == 'y':
            response = requests.get(
                f'{job_processor_url}/v2/call/run/{instance_id}?ticket={ticket}'
            )
            print('response', response.status_code)
            print(response.content)

        is_delete_job = input('delete job? ')
        if is_delete_job is None or is_delete_job == 'y':
            print('deleting job')

            response = requests.delete(f'{job_processor_url}/v2/call/run/{instance_id}?ticket={ticket}')
            print('response', response.status_code)


if __name__ == "__main__":
    main()
