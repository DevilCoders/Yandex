"""
Common cluster delete metadata executor.
"""

from ....providers.aws.iam import AWSIAM, AWSRole
from ....providers.aws.kms import KMS
from ....providers.conductor import ConductorApi
from ....providers.iam import Iam
from ....providers.pillar import DbaasPillar
from ....providers.solomon_client import SolomonApiV2
from ....types import HostGroup
from ....utils import get_first_value
from ...utils import build_host_group
from ..delete_metadata import BaseDeleteMetadataExecutor


class ClusterDeleteMetadataExecutor(BaseDeleteMetadataExecutor):
    """
    Generic class for cluster delete metadata executors
    """

    def __init__(self, config, task, queue, args):
        super().__init__(config, task, queue, args)
        self.conductor = ConductorApi(self.config, self.task, self.queue)
        self.solomon = SolomonApiV2(self.config, self.task, self.queue)
        self.iam = Iam(config, task, queue)
        self.properties = None
        self.pillar = DbaasPillar(config, task, queue)
        self.kms = KMS(self.config, self.task, self.queue)
        self.aws_iam = AWSIAM(self.config, self.task, self.queue)
        self.restartable = True

    def _delete_hosts(self):
        """
        Delete hosts from dns, conductor, deploy api, certificator, and solomon
        """
        host_group = build_host_group(self.properties, self.args['hosts'])
        self._delete_host_group(host_group)

        self.conductor.group_absent(self.task['cid'])

        self.solomon.cluster_absent(self.task['cid'])

    def _delete_service_account(self):
        # TODO: should be refactored to _delete_cluster_service_account method

        # Support for porto will be added in https://st.yandex-team.ru/MDB-14040
        if not self._is_compute(self.args['hosts']) and not self._is_aws(self.args['hosts']):
            return

        service_account_name = 'cluster-agent-' + self.task['cid']
        self.iam.reconnect()
        service_account = self.iam.find_service_account_by_name(
            self.config.per_cluster_service_accounts.folder_id, service_account_name
        )

        if service_account:
            self.iam.delete_service_account(service_account.id)

    def _delete_cluster_service_account(self, host_group: HostGroup):
        if self._is_aws(host_group.hosts):
            self._delete_aws_instance_profile()

    def _delete_aws_instance_profile(self):
        role: AWSRole = self.aws_iam.get_cluster_role()
        self.aws_iam.role_absent(
            role.name,
            self.config.aws_iam.managed_dataplane_policy_arns,
        )

    def _delete_encryption_stuff(self, host_group: HostGroup):
        key = self.pillar.get('cid', self.task['cid'], ['data', 'encryption', 'key'])
        if not key:
            return

        if key['type'] == 'aws':
            # suppose all hosts are going to be deleted in the same region
            region_name = get_first_value(host_group.hosts)['region_name']
            self.kms.key_absent(key['id'], region_name)

    def run(self):
        self._delete_hosts()
