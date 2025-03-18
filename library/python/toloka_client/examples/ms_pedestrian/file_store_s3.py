### File with class FileStorer, to publish files in web and store pipeline results
### You can implement any class with same 4 functions:
### - store_file
### - del_files_by_urls
### - store_main_results
### - store_bonus_results
###
### To check your configuration, you can run this file (first check the configuration at the bottom of the file).

import logging
import aioboto3  #!pip install aioboto3

from datetime import datetime
from io import BytesIO
import itertools
import os
import pandas
import sys
from typing import List
import uuid

if sys.version_info[:2] >= (3, 8):
    from functools import cached_property
else:
    from cached_property import cached_property


class S3FileStorer:
    """To store files on s3 
    Make sure your s3 service account is configured with an admin role"""

    def __init__(self,
                 key_id, access_key, bucket_name, s3_url='https://storage.yandexcloud.net',
                 tmp_folder='tmp',
                 main_folder='main',
                 bonus_folder='main_bonus') -> None:
        self.key_id=key_id # key id
        self.access_key=access_key  #  access key
        self.bucket_name = bucket_name  # existing bucket name
        self.s3_url = s3_url

        # folders in bucket, where to store images
        self.tmp_folder = tmp_folder  # while images uses in toloka projects (scrit will delete all files from this folder)
        self.main_folder = main_folder  # where to store all good files from main tasks part
        self.bonus_folder = bonus_folder  # where to store all good files from bonus fields (related to the organization from the task)

    @cached_property
    def session(self):
        return aioboto3.Session(
            aws_access_key_id=self.key_id,
            aws_secret_access_key=self.access_key,
        )

    async def test_call(self):
        async with self.session.client("s3", endpoint_url=self.s3_url) as s3:
            print('Existing buckets:')
            buckets = await s3.list_buckets()
            for bucket in buckets['Buckets']:
                print(f'  {bucket["Name"]}')
            print(f'\n  Bucket "{self.bucket_name}" {"does not" if self.bucket_name not in buckets["Buckets"] else ""} exists')

            print('\nWrite file')
            with open('./README.md', 'rb') as f:
                file_url = await self.store_file(f, 'README.md')
                print(file_url)
            #await self.del_files_by_urls([file_url])

    async def store_file(self, file: BytesIO, file_name: str) -> str:
        logging.debug('Before session.client("s3"...)')
        async with self.session.client("s3", endpoint_url=self.s3_url) as s3:
            #new_file_name = f'{str(uuid.uuid4())}-{file_name}'
            new_file_name = file_name
            file.seek(0)
            now = datetime.utcnow().strftime("%Y-%m-%d")
            logging.debug('Before upload_fileobj')
            await s3.upload_fileobj(file, self.bucket_name, f'{self.tmp_folder}/{now}/{new_file_name}')
            logging.debug('After upload_fileobj')
        return f'{self.s3_url}/{self.bucket_name}/{self.tmp_folder}/{now}/{new_file_name}'

    async def del_files_by_urls(self, urls: List[str]) -> None:
        async with self.session.client("s3", endpoint_url=self.s3_url) as s3:
            objects_to_delete = []
            url_prefix = f'{self.s3_url}/{self.bucket_name}/'
            for url in urls:
                if url.startswith(url_prefix):
                    objects_to_delete.append({'Key': url[len(url_prefix):]})
                else:
                    # TODO log error
                    pass
            result = await s3.delete_objects(Bucket=self.bucket_name, Delete={'Objects':objects_to_delete})

    async def store_main_no_org_results(self, task_id, pedestrian_task_input) -> None:
        # task_id - id of the task in pedestrian project, could helps you to link answers
        # pedestrian_task_input - all inputs from pedestrian task  {field_name: field_val}
        result_file_name = 'no_org.tsv'
        fields_list = [
            'name',
            'PWSURL',
            'Segment',
            'address',
            'EntityId',
            'ObjectId',
            'Phonumber',
            'coordinates',
        ]
        if not os.path.isfile(result_file_name):
            df = pandas.DataFrame([], columns=fields_list)
        else:
            df = pandas.read_csv(result_file_name, sep='\t')

        value_list = [pedestrian_task_input[field_name] if field_name in pedestrian_task_input else '' for field_name in fields_list]
        df = df.append(pandas.Series(value_list, index=df.columns), ignore_index=True)

        df.to_csv(result_file_name, sep='\t', index=False)


    async def store_main_results(self, task_id, pedestrian_task_input, pedestrian_task_answers, images_dict) -> None:
        # task_id - id of the task in pedestrian project, could helps you to link answers
        # pedestrian_task_input - all inputs from pedestrian task  {field_name: field_val}
        # pedestrian_task_answers - all answers from pedestrian task, except files {field_name: field_val}
        # images_dict - links on good files, could be several url in one field {field_name: [url]}

        # move all files on s3 to another folder
        url_prefix = f'{self.s3_url}/{self.bucket_name}/'
        async with self.session.client("s3", endpoint_url=self.s3_url) as s3:
            for image_urls in images_dict.values():
                for url in image_urls:
                    if not url.startswith(url_prefix):
                        continue
                    file_name = url[len(url_prefix):]
                    copy_source = {'Bucket': self.bucket_name, 'Key': file_name}
                    await s3.copy_object(CopySource=copy_source, Bucket=self.bucket_name, Key=file_name.replace(self.tmp_folder, self.main_folder))
                    # Do not delete images from 'tmp', so we can look at them and understand why it accepted or rejected
                    # await s3.delete_object(Bucket=self.bucket_name, Key=file_name)

        for group_name in images_dict:
            new_names = [
                file_name.replace(f'/{self.tmp_folder}/', f'/{self.main_folder}/')
                for file_name in images_dict[group_name]
            ]
            images_dict[group_name] = new_names

        result_tsv_file_name = 'main_tasks.tsv'

        input_fields = ['name', 'PWSURL', 'Segment', 'address', 'EntityId', 'ObjectId', 'Phonumber', 'coordinates']
        output_fields = [ #'comment','comment_hoo',
            'can_take_hoo', 'comment_menu', 'can_take_menu', 'comment_covid', 'is_restaurant',
            'reason_no_hoo', 'can_take_covid', 'reason_no_menu', 'reason_no_covid', 'comment_interior', 'comment_outdoors',
            'outside_of_hours', 'reason_no_photos', 'can_take_interior', 'can_take_outdoors', 'reason_no_interior',
            'reason_no_outdoors', 'worker_coordinates'
        ]
        images_fields = ['name_business', 'imgs_name_address', 'imgs_facade', 'hoo_business']

        prepared_dict = {}
        for field_name in input_fields:
            prepared_dict[field_name] = pedestrian_task_input[field_name] if field_name in pedestrian_task_input else None

        for field_name in output_fields:
            prepared_dict[field_name] = pedestrian_task_answers[field_name] if field_name in pedestrian_task_answers else None

        for field_name in images_fields:
            prepared_dict[field_name] = images_dict[field_name] if field_name in images_dict else None

        new_df = pandas.DataFrame([prepared_dict.values()], columns=prepared_dict.keys())
        if os.path.isfile(result_tsv_file_name):
            df = pandas.read_csv(result_tsv_file_name, sep='\t')
            df = df.append(new_df, ignore_index=True)
        else:
            df = new_df
        df.to_csv(result_tsv_file_name, sep='\t', index=False)

    async def store_bonus_results(self, task_id, pedestrian_task_input, image_type, images_list) -> None:
        # task_id - id of the task in pedestrian project, could helps you to link answers
        # pedestrian_task_input - all inputs from pedestrian task  {field_name: field_val}
        # image_type - name of field in pedestrian project output with images, for example 'imgs_menu'
        # images_list - links on good files 

        # for example you can move files from S3 to somwhere else, or to another folder on s3
        # move all files on s3 to another folder
        url_prefix = f'{self.s3_url}/{self.bucket_name}/'
        async with self.session.client("s3", endpoint_url=self.s3_url) as s3:
            for url in images_list:
                if not url.startswith(url_prefix):
                    continue
                file_name = url[len(url_prefix):]
                copy_source = {'Bucket': self.bucket_name, 'Key': file_name}
                await s3.copy_object(CopySource=copy_source, Bucket=self.bucket_name, Key=file_name.replace(self.tmp_folder, self.bonus_folder))
                # Do not delete images from 'tmp', so we can look at them and understand why it accepted or rejected
                # await s3.delete_object(Bucket=self.bucket_name, Key=file_name)

        new_url_list = [
            file_name.replace(f'/{self.tmp_folder}/', f'/{self.bonus_folder}/')
            for file_name in images_list
        ]

        result_tsv_file_name = 'bonus_tasks.tsv'
        prepared_dict = {}

        input_fields = ['name', 'PWSURL', 'Segment', 'address', 'EntityId', 'ObjectId', 'Phonumber', 'coordinates']

        for field_name in input_fields:
            prepared_dict[field_name] = pedestrian_task_input[field_name] if field_name in pedestrian_task_input else None

        prepared_dict['bonus_type'] = image_type
        prepared_dict['images'] = new_url_list

        new_df = pandas.DataFrame([prepared_dict.values()], columns=prepared_dict.keys())
        if os.path.isfile(result_tsv_file_name):
            df = pandas.read_csv(result_tsv_file_name, sep='\t')
            df = df.append(new_df, ignore_index=True)
        else:
            df = new_df
        df.to_csv(result_tsv_file_name, sep='\t', index=False)        


if __name__ == '__main__':
    import asyncio
    import keyring # pip install keyring
    print('Check connection. And get all buckets names.')
    storer = FileStorer(
        key_id=keyring.get_password('TOLOKA', 's3_key_id_tulinev'),
        access_key=keyring.get_password('TOLOKA', 's3_access_key_tulinev'),
        bucket_name='pedestrian-project-60846',  # be careful buckets must have unique names in all cloud
        s3_url='https://storage.yandexcloud.net',
    )
    
    asyncio.run(storer.test_call())
    print('End')
