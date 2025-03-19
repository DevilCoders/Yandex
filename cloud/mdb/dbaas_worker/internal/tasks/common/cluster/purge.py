"""
Common cluster delete executor.
"""
from ....providers.metadb_backup_service import MetadbBackups
from ....providers.s3_bucket import S3Bucket
from ....providers.s3_bucket_access import S3BucketAccess
from ..base import BaseExecutor


class ClusterPurgeExecutor(BaseExecutor):
    """
    Generic class for cluster purge executors.
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.s3_bucket = S3Bucket(config, task, queue)
        self.s3_bucket_access = S3BucketAccess(config, task, queue)
        self.backup_db = MetadbBackups(self.config, self.task, self.queue)
        self.properties = None
        self.restartable = True

    def run(self):
        """
        Delete backups bucket
        """
        self.backup_db.delete_cluster_backups(cid=self.task['cid'])

        if self.args.get('s3_buckets'):
            for endpoint, bucket in self.args['s3_buckets'].items():
                self.s3_bucket.absent(bucket, endpoint)
                self.s3_bucket_access.creds_absent(endpoint)
            return

        # TODO: Remove after int-api and worker will both work with s3_buckets task argument.
        bucket = self.args['s3_bucket']
        self.s3_bucket.absent(bucket)
