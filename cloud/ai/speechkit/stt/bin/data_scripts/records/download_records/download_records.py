import boto3
import json
from multiprocessing.pool import ThreadPool
import os
from pydub import AudioSegment

import nirvana.job_context as nv

from cloud.ai.speechkit.stt.lib.experiments.audio.make_audio_segment import make_audio_segment


def main():
    job_context = nv.context()
    inputs = job_context.get_inputs()
    outputs = job_context.get_outputs()
    parameters = job_context.get_parameters()

    records_path = inputs.get('records')
    audio_path = outputs.get('audio')

    output_sampling_rate = parameters.get('output_sampling_rate')
    output_format = parameters.get('output_format')

    with open(records_path) as f:
        records = json.load(f)

    save_dir = 'audio'
    download_records(records, output_sampling_rate, output_format, save_dir)

    os.system(f'tar czf {audio_path} {save_dir}')


def download_records(records, output_sampling_rate, output_format, save_dir):
    os.system(f'mkdir {save_dir}')

    session = boto3.session.Session()
    s3 = session.client(service_name='s3', endpoint_url='https://storage.yandexcloud.net')

    def download(record):
        path = os.path.join(save_dir, record['id'])
        if 'spec' in record:
            data = make_audio_segment(record, s3, output_sampling_rate)
            data.export(out_f=path, format=output_format)
        else:
            s3_object = s3.get_object(Bucket=record['s3_obj']['bucket'], Key=record['s3_obj']['key'])
            with open(path, 'wb') as out_f:
                out_f.write(s3_object['Body'].read())

    pool = ThreadPool(processes=16)
    pool.map(download, records)
