import nirvana.job_context as nv
import pair_dataset
from multiprocessing.pool import ThreadPool
import tarfile
import os
import json


job_context = nv.context()
inputs = job_context.get_inputs()
outputs = job_context.get_outputs()
nv_params = job_context.get_parameters()

tar = tarfile.open(inputs.get('tasks'))
tar.extractall()
tar.close()

markup_name = './tasks-dataset.json'

s3_config = {}
s3_config['s3_endpoint_url'] = 'https://storage.yandexcloud.net'
s3_config['aws_access_key_id'] = nv_params.get('data-sa-aws-access-key-id')
s3_config['aws_secret_access_key'] = nv_params.get('data-sa-aws_secret_access_key')


markup = pair_dataset.PairDataset(markup_name, s3_config)

thread_pool = ThreadPool(processes=8)
s3 = markup.upload_to_s3_questions('./_s3.json', nv_params.get('s3_data_bucket'), nv_params.get('s3_data_path'), thread_pool)

s3.get_tasks_mos(outputs.get('tasks'))

with open(outputs.get('control_tasks'), 'w', encoding='utf-8') as f:
    json.dump({'dummy': 'empty_json'}, f, ensure_ascii=False, indent=4)
