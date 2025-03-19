import yt.wrapper as yt
import argparse
import os

from scipy.io import wavfile
import io
import json
from audio import AudioSample
import boto3

yt.config["proxy"]["url"] = "hahn"

class YtReader:
    def __init__(self, table_path, start_index=0, end_index=None):
        self._table_path = table_path
        self._iterator = None

        self._start_index = start_index
        self._end_index = end_index

    def _reset_reader(self):
        table = yt.TablePath(name=self._table_path, start_index=self._start_index, end_index=self._end_index)
        self._iterator = yt.read_table(table=table, format=yt.YsonFormat(encoding=None))

    def __next__(self):
        if self._iterator is None:
            self._reset_reader()

        try:
            return next(self._iterator)
        except StopIteration:
            raise StopIteration

    def __iter__(self):
        return self


def load_from_s3_and_save(s3, bucket, key, path):
    buffer = io.BytesIO()
    s3.download_fileobj(bucket, key, buffer)
    buffer.seek(0)
    return AudioSample(file=buffer).save(path)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--input_table', type=str, required=True)
    parser.add_argument('--output_folder', type=str, required=True)
    parser.add_argument('--output_json', type=str, required=True)
    parser.add_argument('--mode', type=str, required=True)
    parser.add_argument('--limit', type=int, default=10000)
    parser.add_argument('--yt_token', type=str, required=False)

    args = parser.parse_args()

    if args.mode not in ['single', 's3_pair']:
        print(f'Unknown mode {args.mode}')
        return

    if args.yt_token is not None:
        yt.config["token"] = args.yt_token

    yt_reader = YtReader(args.input_table)

    dataset = []
    count = 0
    if args.mode == 's3_pair':
        s3_config = {}
        s3_config['s3_endpoint_url'] = 'https://storage.yandexcloud.net'
        s3_config['aws_access_key_id'] = os.environ['cloud-ai-data-aws_access_key_id']
        s3_config['aws_secret_access_key'] = os.environ['cloud-ai-data-aws_secret_access_key']
        s3_config['bucket'] = 'cloud-ai-data'

        session = boto3.session.Session()

        s3 = session.client(
            service_name='s3',
            endpoint_url=str(s3_config['s3_endpoint_url']),
            aws_access_key_id=str(s3_config['aws_access_key_id']),
            aws_secret_access_key=str(s3_config['aws_secret_access_key'])
        )

    for yt_row in yt_reader:
        if count == args.limit:
            break

        if count % 100 == 0 and count > 0:
            print(f'Iteration #{count}')

        cur_id = yt_row[b'ID'].decode("utf-8")
        if args.mode == 'single':
            cur_text = yt_row[b'text'].decode("utf-8")
            sample_rate, data = wavfile.read(io.BytesIO(yt_row[b'pcm__wav']))

            wavfile.write(args.output_folder + '/' + cur_id + '.wav', sample_rate, data)

            processed_row = {'uuid': cur_id, 'text': cur_text, 'reference': args.output_folder + '/' + cur_id + '.wav'}

        if args.mode == 's3_pair':
            s3_reference_path = yt_row[b's3_reference_path'].decode("utf-8")
            s3_synthesis_path = yt_row[b's3_synthesis_path'].decode("utf-8")

            ref_path = args.output_folder + '/' + cur_id + '_reference.wav'
            syn_path = args.output_folder + '/' + cur_id + '_synthesis.wav'

            load_from_s3_and_save(s3, s3_config['bucket'], s3_reference_path,
                                  ref_path)
            load_from_s3_and_save(s3, s3_config['bucket'], s3_synthesis_path,
                                  syn_path)

            processed_row = {'uuid': cur_id, 'reference': ref_path, 'synthesis': syn_path}


        dataset.append(processed_row)
        count += 1

    with open(args.output_json, 'w', encoding='utf-8') as f:
        json.dump(dataset, f, ensure_ascii=False, indent=4)

main()



