import requests
import json
import logging
import time
from uuid import uuid4
import nirvana.job_context as nv

logging.basicConfig()
logging.getLogger().setLevel(logging.DEBUG)
requests_log = logging.getLogger("requests.packages.urllib3")
requests_log.setLevel(logging.DEBUG)
requests_log.propagate = True


def main():
    job_context = nv.context()
    parameters = job_context.get_parameters()
    inputs = job_context.get_inputs()

    testing_job_processor_path = parameters.get("job-processor")
    sleep_time = parameters.get("sleep")
    max_retries_on_update = parameters.get("max-retries-on-update")
    testing_job_processor_url = f'http://{testing_job_processor_path}/api'
    request_path = inputs.get("request.json")

    with open(request_path, 'r') as f:
        job_json = json.load(f)

    instance_id = str(uuid4())

    print(f'\n\ninstance_id: {instance_id}', flush=True)

    retries_on_update = 0
    while True:
        response = requests.put(
            f'{testing_job_processor_url}/v2/call/run/{instance_id}',
            data=json.dumps(job_json),
            headers={'Content-Type': 'application/json'}
        )

        print(f'response {response.status_code}')

        if response.status_code == 200:
            time.sleep(sleep_time)
            retries_on_update = 0
            result = response.json()
            print(result, flush=True)

            ticket = result['ticket']
            print(f'ticket: {ticket}', flush=True)

            while True:
                response = requests.get(
                    f'{testing_job_processor_url}/v2/call/run/{instance_id}?ticket={ticket}'
                )
                try:
                    response_json = json.loads(response.content.decode("ascii"))
                except:
                    time.sleep(1)
                    continue

                job_status = response_json['status']
                print(f'Job status: {job_status}', flush=True)

                if response_json['status'] == "FAILED":
                    print("TEST FAILED!", flush=True)
                    exit(1)
                elif response_json['status'] == "FINISHED":
                    print("SUCCESS!", flush=True)
                    break

        elif response.status_code == 503:
            time.sleep(sleep_time)
            print("SERVICE UNAVAILABLE!", flush=True)
            retries_on_update += 1
            if retries_on_update == max_retries_on_update:
                print("MAX RETRIES EXPIRED!", flush=True)
                exit(1)

            continue

        else:
            print("TEST FAILED!", flush=True)
            exit(1)


if __name__ == "__main__":
    main()
