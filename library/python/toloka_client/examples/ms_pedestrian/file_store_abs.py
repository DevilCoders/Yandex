### File with class FileStorer, to publish files in web and store pipeline results in AZURE BLOB STORAGE
### You can implement any class with same 4 functions:
### - store_file
### - del_files_by_urls
### - store_main_results
### - store_bonus_results
###
### To check your configuration, you can run this file (first check the configuration at the bottom of the file).

import logging
from azure.storage.blob.aio import BlobServiceClient #!pip install azure-storage-blob
from azure.storage.blob import generate_blob_sas, BlobSasPermissions

from datetime import datetime, timedelta
from io import BytesIO
import itertools
import os
import pandas
import sys
from typing import List
from urllib.parse import urlparse
import uuid


class AzureFileStorer:
    """To store files on azure blob storage"""

    def __init__(self,
                 account_key, account_name, container_name,
                 sas_valid_days=None, # if parametr is None, will return permanent links to files
                 tmp_folder='tmp',
                 main_folder='main',
                 bonus_folder='main_bonus') -> None:
        self.account_key = account_key
        self.account_name = account_name
        self.container_name = container_name  # existing container
        self.connection_string = f'DefaultEndpointsProtocol=https;AccountName={self.account_name};AccountKey={self.account_key};EndpointSuffix=core.windows.net'

        self.sas_valid_days = sas_valid_days

        # folders in container, where to store images
        self.tmp_folder = tmp_folder  # while images uses in toloka projects (scrit will delete all files from this folder)
        self.main_folder = main_folder  # where to store all good files from main tasks part
        self.bonus_folder = bonus_folder  # where to store all good files from bonus fields (related to the organization from the task)

    def blob_service_client(self):
        return BlobServiceClient.from_connection_string(self.connection_string)

    async def test_call(self):
        async with self.blob_service_client() as serv_client:
            print('Existing containers:')
            all_containers = []
            async for container in serv_client.list_containers(include_metadata=True):
                print(f'  {container["name"]}')
                all_containers.append(container["name"])

            print(f'\n  Container "{self.container_name}" {"does not" if self.container_name not in all_containers else ""} exists')

            print('\nWrite file')
            with open('./README.md', 'rb') as f:
                file_url = await self.store_file(f, 'README.md')
                print(file_url)

    async def store_file(self, file: BytesIO, file_name: str) -> str:
        ### stores file in "tmp_folder" and splits them by days
        logging.debug('Before session.client("s3"...)')
        async with self.blob_service_client() as serv_client:
            #new_file_name = f'{str(uuid.uuid4())}-{file_name}'
            new_file_name = file_name
            file.seek(0)
            now = datetime.utcnow().strftime("%Y-%m-%d")
            path_to_file = f'{self.tmp_folder}/{now}/{new_file_name}'
            logging.debug('Before upload_fileobj')
            blob_client = serv_client.get_blob_client(container=self.container_name, blob=path_to_file)
            await blob_client.upload_blob(file)
            logging.debug('After upload_fileobj')

        result_url = f'https://{self.account_name}.blob.core.windows.net/{self.container_name}/{path_to_file}'
        if self.sas_valid_days is not None:
            sas_token = generate_blob_sas(
                account_name=self.account_name, 
                container_name=self.container_name,
                blob_name=path_to_file,
                account_key=self.account_key,
                permission=BlobSasPermissions(read=True),
                expiry=datetime.utcnow() + timedelta(days=self.sas_valid_days)
            )
            result_url += f'?{sas_token}'
        return result_url

    async def del_files_by_urls(self, urls: List[str]) -> None:
        async with self.blob_service_client() as serv_client:
            container_client = serv_client.get_container_client(self.container_name)
            objects_to_delete = []
            url_prefix = f'https://{self.account_name}.blob.core.windows.net/{self.container_name}/'
            for url in urls:
                if url.startswith(url_prefix):
                    objects_to_delete.append(url[len(url_prefix):])
                else:
                    # TODO log error
                    pass
            await container_client.delete_blobs(*objects_to_delete)

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

        # move all files to another folder
        #url_prefix = f'https://{self.account_name}.blob.core.windows.net/{self.container_name}/'
        url_prefix = f'/{self.container_name}/'
        async with self.blob_service_client() as serv_client:
            for group_name, image_urls in images_dict.items():
                new_names = []
                for url in image_urls:
                    url_path = urlparse(url).path
                    if not url_path.startswith(url_prefix):
                        continue
                    file_name = url_path[len(url_prefix):]
                    new_file_name = file_name.replace(self.tmp_folder, self.main_folder)
                    copied_blob = serv_client.get_blob_client(self.container_name, new_file_name)
                    await copied_blob.start_copy_from_url(f'https://{self.account_name}.blob.core.windows.net{url_path}')
                    new_names.append(f'https://{self.account_name}.blob.core.windows.net/{self.container_name}/{new_file_name}')
                    # Do not delete images from 'tmp', so we can look at them and understand why it was accepted or rejected
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

        # move all files to another folder
        #url_prefix = f'https://{self.account_name}.blob.core.windows.net/{self.container_name}/'
        url_prefix = f'/{self.container_name}/'
        new_url_list = []
        async with self.blob_service_client() as serv_client:

            for url in images_list:
                url_path = urlparse(url).path
                if not url_path.startswith(url_prefix):
                    continue
                file_name = url_path[len(url_prefix):]
                new_file_name = file_name.replace(self.tmp_folder, self.bonus_folder)
                copied_blob = serv_client.get_blob_client(self.container_name, new_file_name)
                await copied_blob.start_copy_from_url(f'https://{self.account_name}.blob.core.windows.net{url_path}')
                new_url_list.append(f'https://{self.account_name}.blob.core.windows.net/{self.container_name}/{new_file_name}')
                # Do not delete images from 'tmp', so we can look at them and understand why it was accepted or rejected

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
    print(keyring.get_password('TOLOKA', 'tulinev_azure_blob_storage_key'))
    storer = AzureFileStorer(
        account_key=keyring.get_password('TOLOKA', 'tulinev_azure_blob_storage_key'),
        account_name='tevexperiments',
        container_name='ms-pedestrian',
        sas_valid_days=1,
    )

    asyncio.run(storer.test_call())
    print('End')
