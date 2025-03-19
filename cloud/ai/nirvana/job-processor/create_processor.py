import os
from nirvana_api import NirvanaApi


AUTHORIZATIONS = [
    {"action": "READ", "roles": ["nirvana.operation.cloudml"]},
    {"action": "MODIFY", "roles": ["nirvana.operation.cloudml"]}
]

CAPABILITIES = [
    'INTERFACE_V2',
    'VALIDATE_OPERATION',
    'REPLICATED_LOGS'
]


def create_processor(
    nirvana,
    code,
    name,
    description,
    service_url,
    documentation_url,
    mailing_list,
    bug_tracker_queue,
    capabilities,
    authorizations
):
    return nirvana._request(
        'createProcessor',
        dict(
            code=code,
            parameters=dict(
                name=name,
                description=description,
                serviceUrl=service_url,
                documentationUrl=documentation_url,
                mailingList=mailing_list,
                bugTrackerQueue=bug_tracker_queue,
                capabilities=capabilities,
                authorizations=authorizations
            )
        )
    )


def create_cloud_processor(nirvana_api):
    result = create_processor(
        nirvana_api,
        code='cloud',
        name='Cloud',
        description='JobProcessor that schedule computations on Yandex.Cloud Compute machines',
        service_url='localhost',
        documentation_url='https://wiki.yandex-team.ru/users/d-kruchinin/CloudProcessor/',
        mailing_list='',
        bug_tracker_queue='CLOUD',
        capabilities=CAPABILITIES,
        authorizations=AUTHORIZATIONS
    )
    print(result)

def update_processor(
    nirvana,
    processor_id,
    name,
    description,
    service_url,
    documentation_url,
    mailing_list,
    bug_tracker_queue,
    capabilities,
    authorizations
):
    return nirvana._request(
        'updateProcessor',
        dict(
            processorId=processor_id,
            parameters=dict(
                name=name,
                description=description,
                serviceUrl=service_url,
                documentationUrl=documentation_url,
                mailingList=mailing_list,
                bugTrackerQueue=bug_tracker_queue,
                capabilities=capabilities,
                authorizations=authorizations
            )
        )
    )


def update_cloud_processor(nirvana_api, processor_id, service_url):
    update_processor(
        nirvana=nirvana_api,
        processor_id=processor_id,
        name='Cloud',
        description='JobProcessor that schedule computations on Yandex.Cloud Compute machines',
        service_url=service_url,
        documentation_url='https://wiki.yandex-team.ru/users/d-kruchinin/CloudProcessor/',
        mailing_list='',
        bug_tracker_queue='CLOUD',
        capabilities=CAPABILITIES,
        authorizations=AUTHORIZATIONS
    )


def create_processor_operation(
    nirvana,
    processor_id,
    code,
    name,
    description,
    block_type_template_id,
    authorizations
):
    return nirvana._request(
        'createProcessorOperation',
        dict(
            processorId=processor_id,
            code=code,
            parameters=dict(
                name=name,
                description=description,
                blockTypeTemplateId=block_type_template_id,
                authorizations=authorizations
            )
        )
    )


def create_cloud_processor_operation(nirvana_api, processor_id):
    create_processor_operation(
        nirvana_api,
        processor_id=processor_id,
        code='run',
        name='Run',
        description='ProcessorOperation to run single machine jobs on Cloud instances',
        block_type_template_id='',
        authorizations=AUTHORIZATIONS
    )


def main():
    with open(os.path.expanduser('~/.secrets/oauth_nirvana_api')) as f:
        token = f.read()[:-1]

    PROD_PROCESSOR_ID = '9e74be2c-9c53-4621-8a40-835cb5e39283'
    TESTING_PROCESSOR_ID = '3abe0b58-a345-4645-b08d-fde38453033b'
    nv_env = input('prod or testing: ')

    nirvana_url = {
        'prod': 'nirvana.yandex-team.ru',
        'testing': 'test.nirvana.yandex-team.ru'
    }[nv_env]

    processor_id = {
        'prod': PROD_PROCESSOR_ID,
        'testing': TESTING_PROCESSOR_ID
    }[nv_env]

    nirvana_api = NirvanaApi(
        server=nirvana_url,
        oauth_token=token
    )

    print('Username:')
    print(nirvana_api.get_username())

    #print('creating cloud processor')
    #create_cloud_processor(nirvana_api)


    # print('creating cloud processor operation')
    # create_cloud_processor_operation(nirvana_api, PROCESSOR_ID)
    # SERVICE_URL = 'http://[2a02:6b8:c02:900:0:f816:0:21b]:6000/api' # ctulhu
    service_url = input('job processor url: ')

    print('updating cloud processor')
    print('nirvana_url: ', nirvana_url)
    print('processor_id: ', processor_id)
    print('service_url: ', service_url)
    update_cloud_processor(nirvana_api, processor_id, service_url)


if __name__ == "__main__":
    main()

