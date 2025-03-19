from typing import Generator, Optional

from cloud.mdb.internal.python import grpcutil
from cloud.mdb.internal.python.grpcutil.exceptions import NotFoundError
from cloud.mdb.internal.python.compute.disks import DiskModel, DisksClient, DisksClientConfig
from cloud.mdb.internal.python.compute.instances import (
    InstanceModel,
    InstanceView,
    InstancesClient,
    InstancesClientConfig,
)
from cloud.mdb.internal.python.logs import MdbLoggerAdapter

from cloud.mdb.infratests.test_helpers.context import Context


class ComputeApi:
    def __init__(self, context: Context):
        self.config = context.test_config
        self.logger = MdbLoggerAdapter(context.logger, extra={})
        self._instance_id_cache = dict()
        error_handlers = {}
        transport = grpcutil.Config(
            url=self.config.iaas.compute_url,
            cert_file=self.config.ca_path,
        )

        def token_getter():
            return context.user_iam_token

        self.__disks_client = DisksClient(
            config=DisksClientConfig(transport=transport),
            logger=self.logger,
            token_getter=token_getter,
            error_handlers=error_handlers,
        )
        self.__instances_client = InstancesClient(
            config=InstancesClientConfig(transport=transport),
            logger=self.logger,
            token_getter=token_getter,
            error_handlers=error_handlers,
        )

    def get_disk(self, disk_id) -> Optional[DiskModel]:
        """
        Get disk by id
        """
        return self.__disks_client.get_disk(disk_id)

    def get_instance(
        self, fqdn: str = None, instance_id: str = None, folder_id: str = None, view='BASIC'
    ) -> Optional[InstanceModel]:
        """
        Get instance info if exists
        """
        with self.logger.context(method='get_instance', fqdn=fqdn, instance_id=instance_id):
            instance_id = instance_id or self._instance_id_cache.get(fqdn)
            if instance_id:
                try:
                    instance = self.__instances_client.get_instance(instance_id, InstanceView[view])
                except NotFoundError as exc:
                    self.logger.info('Unable to get instance %s by id: %s', fqdn, repr(exc))
                else:
                    if fqdn and not self._instance_id_cache.get(fqdn):
                        self._instance_id_cache[fqdn] = instance.id
                    return instance

            instances = self.list_instances(folder_id=folder_id, name=_get_compute_name_from_fqdn(fqdn))
            for instance in instances:
                if instance.fqdn == fqdn:
                    self._instance_id_cache[fqdn] = instance.id
                    if view != 'BASIC':
                        return self.get_instance(fqdn=None, instance_id=instance.id, folder_id=folder_id, view=view)
                    return instance
        return None

    def list_instances(self, folder_id: str = None, name: str = None) -> Generator[InstanceModel, None, None]:
        """
        Get instances in folder
        """
        return self.__instances_client.list_instances(
            folder_id or self.config.folder_id,
            name=name,
        )


def get_compute_api(context: Context):
    return ComputeApi(context)


def _get_compute_name_from_fqdn(fqdn):
    """
    Compute does not use fqdn as instance name.
    So we strip domain name from fqdn here
    """
    return fqdn.split('.', maxsplit=1)[0]
