"""
class S3:
s3 funcs mixin
    def cloud_get_by_bucket(self, bucket_name)
    def get_all_buckets(self, cloud_id)
    def move_bucket(self, bucket, folder)
"""
from helpers import *

class St_S3:
    def cloud_get_by_bucket(self, bucket_name):
        headers = {'X-YaCloud-SubjectToken': self.iam_token}
        r = requests.get(self.endpoints.s3_url + bucket_name, headers=headers)
        response = r.json()
        logging.debug('513')
        logging.debug(response)

        if response.get('cloud_id'):
            return response
        else:
            logging.error(response.get('detail'))
            return None

    def get_all_buckets(self, cloud_id):
        result = []
        headers = {'X-YaCloud-SubjectToken': self.iam_token}
        for folder in self.folders_get_by_cloud(cloud_id).folders:
            #folder_id = folder.get('folder_id')
            folder_id = folder.id
            url = self.endpoints.s3_url + '?service_id={folder}'.format(folder=folder_id)
            r = requests.get(url, headers=headers)
            response = r.json()
            logging.debug('531')
            logging.debug(response)
            if len(response.get('results', [])) > 0:
                result.extend(response.get('results'))
            elif response.get('detail'):
                logging.error(response.get('detail'))


        return result

    def move_bucket(self, bucket, folder):

        headers = {'X-YaCloud-SubjectToken': self.iam_token}
        url = self.endpoints.priv_s3_api + '/management/bucket/{}/move'.format(bucket)
        payload = {'service_id': folder}
        r = requests.post(url, json=payload, headers=headers)
        response = r.json()
        logging.debug(response)

        if r.status_code != 200:
            logging.error(response)
        else:
            print('Done. Bucket "{}" moved to folder {}'.format(bucket, folder))
