from ....exceptions import ExposedException
from ...common import BaseProvider, Change

from typing import Union, List
from dbaas_common import tracing, retry

import botocore.exceptions
from botocore.config import Config
from ..sts import get_assume_role_session


class KMSAPIError(ExposedException):
    """
    Base KMS error
    """


class KMSDisabledError(RuntimeError):
    """
    KMS provider not initialized. Enable it in config'
    """


class AliasNotFound(KMSAPIError):
    pass


class InvalidArnException(KMSAPIError):
    pass


DEFAULT_ALLOWED_OPERATIONS = [
    'Decrypt',
    'Encrypt',
]
KEY_ALIAS_TAG_NAME = 'kms.key.alias'
KEY_ID_TAG_NAME = 'kms.key.id'
ROLE_ARN_TAG_NAME = 'kms.role.arn'
ROLE_OPERATIONS_TAG_NAME = 'kms.role.operations'
CONTEXT_KEY = 'kms.key'


class _RegionalKMS(BaseProvider):
    def __init__(self, config, task, queue, region_name):
        super().__init__(config, task, queue)
        session = get_assume_role_session(
            config=config,
            task=task,
            queue=queue,
            aws_access_key_id=config.aws.access_key_id,
            aws_secret_access_key=config.aws.secret_access_key,
            region_name=region_name,
        )

        self._kms = session.client(
            'kms',
            config=Config(
                retries=config.kms.retries,
            ),
        )

    # CreateGrant can fail with strange error
    """
    botocore.errorfactory.InvalidArnException:
      An error occurred (InvalidArnException) when calling the CreateGrant operation:
        ARN does not refer to a valid principal:
          arn:aws:iam::785415555008:role/mdbclusters/mdb-dataplane-node-chcpl92g1kqkd7ogicab
    """
    # Maybe it fails only just after IAM role creation.
    # But role has no state to check.
    # So, let's retry.
    @retry.on_exception(InvalidArnException, max_tries=5, max_wait=60, factor=10)
    @tracing.trace("Grant access")
    def _grant_access(self, key_id: str, role_arn: str, operations: Union[List, None] = None) -> None:
        if operations is None:
            operations = DEFAULT_ALLOWED_OPERATIONS

        tracing.set_tag(KEY_ID_TAG_NAME, key_id)
        tracing.set_tag(ROLE_ARN_TAG_NAME, role_arn)
        tracing.set_tag(ROLE_OPERATIONS_TAG_NAME, operations)

        try:
            self._kms.create_grant(  # type: ignore
                KeyId=key_id,
                GranteePrincipal=role_arn,
                Operations=operations,
            )
        except botocore.exceptions.ClientError as client_error:
            if client_error.response['Error']['Code'] == 'InvalidArnException':
                raise InvalidArnException(str(client_error))
            raise

    @tracing.trace("Create key alias")
    def _create_key_alias(self, key_id: str, alias: str) -> None:
        tracing.set_tag(KEY_ID_TAG_NAME, key_id)
        tracing.set_tag(KEY_ALIAS_TAG_NAME, alias)

        self._kms.create_alias(  # type: ignore
            AliasName=alias,
            TargetKeyId=key_id,
        )

    @tracing.trace("Create key")
    def _create_key(self) -> str:
        context = self.context_get(CONTEXT_KEY)
        if context:
            key_id = context['id']
            return key_id

        data = self._kms.create_key(Tags=self._get_tags())  # type: ignore
        key_id = self._get_key_id_from_response(data)
        return key_id

    @tracing.trace("Delete key")
    def _delete_key(self, key_id: str):
        tracing.set_tag(KEY_ID_TAG_NAME, key_id)
        self._kms.schedule_key_deletion(KeyId=key_id)  # type: ignore

    @staticmethod
    def _get_key_id_from_response(response: dict) -> str:
        key_id = response.get('KeyMetadata', {}).get('KeyId')
        if key_id is None:
            raise KMSAPIError(f"Can not get KMS KeyID, response: {str(response)}")
        return key_id

    @staticmethod
    def _get_key_state_from_response(response: dict) -> str:
        key_state = response.get('KeyMetadata', {}).get('KeyState')
        if key_state is None:
            raise KMSAPIError(f"Can not get KMS KeyState, response: {str(response)}")
        return key_state

    def _get_tags(self) -> List[dict]:
        return [{'TagKey': k, 'TagValue': v} for k, v in self.config.aws.labels.items()]

    @tracing.trace("Get key by alias")
    def _get_key(self, alias: str) -> dict:
        tracing.set_tag(KEY_ALIAS_TAG_NAME, alias)
        try:
            return self._kms.describe_key(KeyId=alias)  # type: ignore
        except botocore.exceptions.ClientError as client_error:
            if client_error.response['Error']['Code'] == 'NotFoundException':
                raise AliasNotFound(f'alias {alias} not found') from client_error
            raise

    @tracing.trace("KMS key exists regional")
    def key_exists(self, alias: str, role_arn: str):
        tracing.set_tag(KEY_ALIAS_TAG_NAME, alias)
        tracing.set_tag(ROLE_ARN_TAG_NAME, role_arn)

        try:
            key = self._get_key(alias)
            key_id = self._get_key_id_from_response(key)
        except AliasNotFound:
            key_id = self._create_key()
            self._create_key_alias(key_id, alias)

        self.add_change(Change(f'kms.key.{key_id}', 'created', {CONTEXT_KEY: {'id': key_id}}))
        self._grant_access(key_id, role_arn)

    @tracing.trace("KMS key absent regional")
    def key_absent(self, alias: str):
        tracing.set_tag(KEY_ALIAS_TAG_NAME, alias)

        try:
            key = self._get_key(alias)
            key_state = self._get_key_state_from_response(key)
            if key_state != 'PendingDeletion':
                self._delete_key(self._get_key_id_from_response(key))
        except AliasNotFound:
            pass

        self.add_change(Change(f'kms.key.{alias}', 'absent'))


class KMS(BaseProvider):
    _kms = None

    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        if config.kms.enabled:
            self._kms = {}

    def _regional_kms(self, region_name: str) -> _RegionalKMS:
        if self._kms is None:
            raise KMSDisabledError

        if region_name not in self._kms:
            self._kms[region_name] = _RegionalKMS(
                self.config,
                self.task,
                self.queue,
                region_name,
            )

        return self._kms[region_name]

    @tracing.trace("KMS key exists")
    def key_exists(self, alias: str, role_arn: str, region_name: str):
        self._regional_kms(region_name).key_exists(alias, role_arn)

    @tracing.trace("KMS key absent")
    def key_absent(self, alias: str, region_name: str):
        self._regional_kms(region_name).key_absent(alias)
