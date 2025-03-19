#!/usr/bin/python3

import os
from multiprocessing.pool import ThreadPool
import ujson as json

import nirvana.job_context as nv

from cloud.ai.lib.python import datetime
from cloud.ai.speechkit.stt.lib.data.model.common import s3_consts
from cloud.ai.speechkit.stt.lib.data_pipeline.files import unpack_and_list_files, obfuscated_bits_dir
from cloud.ai.speechkit.stt.lib.utils.s3 import create_client


def main():
    op_ctx = nv.context()

    inputs = op_ctx.inputs
    outputs = op_ctx.outputs

    records_bits_files_path = inputs.get('records_bits_files.tar.gz')

    records_bits_s3_urls_path = outputs.get('records_bits_s3_urls.json')

    date_dir = datetime.format_datetime_for_s3_subpath(datetime.now())
    s3_key_prefix = os.path.join('Speechkit/STT/Toloka/Assignments', date_dir)

    bucket = s3_consts.data_bucket

    s3 = create_client()

    bit_filenames = unpack_and_list_files(archive_path=records_bits_files_path, directory_path=obfuscated_bits_dir)

    def upload_bit(bit_filename: str) -> str:
        s3_key = os.path.join(s3_key_prefix, bit_filename)
        with open(os.path.join(obfuscated_bits_dir, bit_filename), "rb") as f:
            s3.upload_fileobj(f, Bucket=bucket, Key=s3_key)
        return s3_key

    pool = ThreadPool(processes=16)
    s3_keys = pool.map(upload_bit, bit_filenames)

    bit_filename_to_s3_url = {
        fn: os.path.join(s3_consts.cloud_url, bucket, key) for fn, key in zip(bit_filenames, s3_keys)
    }

    with open(records_bits_s3_urls_path, 'w') as f:
        json.dump(bit_filename_to_s3_url, f, ensure_ascii=False, indent=4)
