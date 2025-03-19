import pair_dataset
from multiprocessing.pool import ThreadPool

if __name__ == "__main__":
    s3_config = {}
    s3_config['s3_endpoint_url'] = 'https://storage.yandexcloud.net'
    s3_config['aws_access_key_id'] = 'lA1lnViCN3jrt_xuBhIm'
    s3_config['aws_secret_access_key'] = '7YSUeWPZmoRigsfLaNs_HmctVPaUCOSKXLo6-uZ7'

    a = pair_dataset.PairDataset('./filipp-dataset.json', s3_config)

    b = a.distort('filipp-dataset-distorted.json', "filipp-distortion", ['SPEED', 'PITCH'])

    thread_pool = ThreadPool(processes=8)

    c = b.upload_to_s3('filipp-dataset-distorted-s3.json', 'cloud-ai-data', 'Tmp/pivr-check', thread_pool)

    c.get_tasks('./tasks.json')
