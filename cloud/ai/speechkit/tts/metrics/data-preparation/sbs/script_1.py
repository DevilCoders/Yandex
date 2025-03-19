import nirvana.job_context as nv
import os
import pair_dataset
from multiprocessing.pool import ThreadPool
import tarfile


job_context = nv.context()
inputs = job_context.get_inputs()
outputs = job_context.get_outputs()
nv_params = job_context.get_parameters()

tar = tarfile.open(inputs.get('tasks'))
tar.extractall()
tar.close()

tar = tarfile.open(inputs.get('control_tasks'))
tar.extractall()
tar.close()

markup_name = './tasks-dataset.json'
honeypots_name = './control-tasks-dataset.json'

os.mkdir('temp_folder')
os.mkdir('temp_folder/honeypots')

s3_config = {}
s3_config['s3_endpoint_url'] = 'https://storage.yandexcloud.net'
s3_config['aws_access_key_id'] = nv_params.get('data-sa-aws-access-key-id')
s3_config['aws_secret_access_key'] = nv_params.get('data-sa-aws_secret_access_key')


honeypots_1 = pair_dataset.PairDataset(honeypots_name, s3_config)
honeypots_2 = honeypots_1.distort_sbs('./temp_folder/_honeypots_sbs.json', './temp_folder/honeypots', ['SPEED', 'PITCH'], ratio_samples_to_distort=1.0)

markup = pair_dataset.PairDataset(markup_name, s3_config)

merged = pair_dataset.PairDataset.merge_list_of_pair_datasets('./temp_folder/_merged.json', [honeypots_2, markup], s3_config)

thread_pool = ThreadPool(processes=8)
merged_s3 = merged.upload_to_s3('./temp_folder/_merged_s3.json', nv_params.get('s3_data_bucket'), nv_params.get('s3_data_path'), thread_pool)

merged_s3.get_tasks_sbs(outputs.get('tasks'), outputs.get('control_tasks'))
