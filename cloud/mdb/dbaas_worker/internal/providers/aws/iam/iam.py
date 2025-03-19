from functools import wraps, cached_property

from ....exceptions import ExposedException
from ...common import BaseProvider, Change

from typing import Callable, List, NamedTuple
from dbaas_common import tracing, retry

import botocore.exceptions
from botocore.config import Config
from ...pillar import DbaasPillar
from ..sts import get_assume_role_session_with_role
from ..byoa import BYOA
from .policies import DEFAULT_BYOA_CLUSTER_ROLE_POLICY, BYOA_ASSUME_ROLE_POLICY_TEMPLATE


class IAMAPIError(ExposedException):
    """
    Base IAM error
    """


class IAMDisabledError(RuntimeError):
    """
    IAM provider not initialized. Enable it in config'
    """


class InvalidPrincipal(IAMAPIError):
    pass


class AWSRole(NamedTuple):
    name: str
    path: str
    arn: str
    instance_profile_arn: str


IAM_ROLE_NAME_TAG_NAME = 'iam.role.name'
IAM_ROLE_PATH_TAG_NAME = 'iam.role.path'
IAM_POLICY_ARN_TAG_NAME = 'iam.policy.arn'
IAM_POLICY_NAME_TAG_NAME = 'iam.policy.name'
IAM_INSTANCE_PROFILE_PATH_TAG_NAME = 'iam.instance_profile.path'
IAM_INSTANCE_PROFILE_NAME_TAG_NAME = 'iam.instance_profile.name'
IAM_USER_TAG_NAME = 'iam.user'

ALREADY_EXISTS_ERROR = 'EntityAlreadyExists'
DOESNT_EXIST_ERROR = 'NoSuchEntity'
LIMIT_EXCEEDED_ERROR = 'LimitExceeded'
MALFORMED_POLICY_DOCUMENT_ERROR = 'MalformedPolicyDocument'

ROLE_PILLAR_PATH = ['data', 'aws', 'role']
INLINE_POLICY_NAME = 'DoubleCloudPolicy'
DOUBLE_CLOUD_CLUSTERS_IAM_PATH = '/DoubleCloud/clusters/'


def check_error(*ok_error_codes: str):
    """
    Decorator for checking AWS errors.
    """

    def wrapper(callback):
        @wraps(callback)
        def check_error_wrapper(*args, **kwargs):
            try:
                return callback(*args, **kwargs)
            except botocore.exceptions.ClientError as client_error:
                if client_error.response['Error']['Code'] not in ok_error_codes:
                    raise

        return check_error_wrapper

    return wrapper


class _Client(BaseProvider):
    def __init__(self, config, task, queue, role_factory):
        super().__init__(config, task, queue)
        session = get_assume_role_session_with_role(
            role_factory=role_factory,
            aws_access_key_id=self.config.aws.access_key_id,
            aws_secret_access_key=self.config.aws.secret_access_key,
            region_name=self.config.aws.region_name,
        )
        self._iam = session.client(
            'iam',
            config=Config(
                retries=config.aws_iam.retries,
            ),
        )

    # create_role can fail with MalformedPolicyDocument error
    """
    botocore.errorfactory.MalformedPolicyDocumentException:
        An error occurred (MalformedPolicyDocument) when calling the CreateRole operation:
            Invalid principal in policy:
                "AWS":"arn:aws:iam::867394682372:role/DoubleCloud/clusters/mdb-dataplane-node-chc3gf5794j4j2jaccpo"
    """
    # It happens when trusted role was just created.
    # But role has no state to check.
    # So, let's retry.
    @retry.on_exception(InvalidPrincipal, max_tries=5, max_wait=60, factor=10)
    @tracing.trace("Create role")
    @check_error(ALREADY_EXISTS_ERROR)
    def create_role(
        self,
        name: str,
        path: str,
        assume_role_policy: str,
    ):
        tracing.set_tag(IAM_ROLE_NAME_TAG_NAME, name)
        tracing.set_tag(IAM_ROLE_PATH_TAG_NAME, path)

        try:
            data = self._iam.create_role(  # type: ignore
                Path=path,
                RoleName=name,
                AssumeRolePolicyDocument=assume_role_policy,
                Tags=self._get_tags(),
            )
        except botocore.exceptions.ClientError as client_error:
            if client_error.response['Error']['Code'] == MALFORMED_POLICY_DOCUMENT_ERROR:
                raise InvalidPrincipal(str(client_error))
            raise
        role_arn = data['Role']['Arn']
        self.add_change(Change(f'iam.role.{role_arn}', 'created'))

    @tracing.trace("Attach role policy")
    def attach_policy(self, role_name: str, policy_arn: str):
        tracing.set_tag(IAM_ROLE_NAME_TAG_NAME, role_name)
        tracing.set_tag(IAM_POLICY_ARN_TAG_NAME, policy_arn)

        self._iam.attach_role_policy(  # type: ignore
            RoleName=role_name,
            PolicyArn=policy_arn,
        )

    @tracing.trace("Create instance profile")
    @check_error(ALREADY_EXISTS_ERROR)
    def create_instance_profile(self, name: str, path: str):
        tracing.set_tag(IAM_INSTANCE_PROFILE_NAME_TAG_NAME, name)
        tracing.set_tag(IAM_INSTANCE_PROFILE_PATH_TAG_NAME, path)

        self._iam.create_instance_profile(  # type: ignore
            Path=path,
            InstanceProfileName=name,
            Tags=self._get_tags(),
        )

    @tracing.trace("Add role to instance profile")
    @check_error(LIMIT_EXCEEDED_ERROR)
    def add_role_to_instance_profile(self, role_name: str, instance_profile_name: str):
        tracing.set_tag(IAM_ROLE_NAME_TAG_NAME, role_name)
        tracing.set_tag(IAM_INSTANCE_PROFILE_NAME_TAG_NAME, instance_profile_name)

        self._iam.add_role_to_instance_profile(  # type: ignore
            RoleName=role_name,
            InstanceProfileName=instance_profile_name,
        )

    @tracing.trace("Delete role")
    @check_error(DOESNT_EXIST_ERROR)
    def delete_role(self, role_name: str):
        tracing.set_tag(IAM_ROLE_NAME_TAG_NAME, role_name)

        self._iam.delete_role(RoleName=role_name)  # type: ignore
        self.add_change(Change(f'iam.role.{role_name}', 'absent'))

    @tracing.trace("Delete instance profile")
    @check_error(DOESNT_EXIST_ERROR)
    def delete_instance_profile(self, name: str):
        tracing.set_tag(IAM_INSTANCE_PROFILE_NAME_TAG_NAME, name)

        self._iam.delete_instance_profile(InstanceProfileName=name)  # type: ignore

    @tracing.trace("Remove role from instance profile")
    @check_error(DOESNT_EXIST_ERROR)
    def remove_role_from_instance_profile(self, name: str):
        tracing.set_tag(IAM_INSTANCE_PROFILE_NAME_TAG_NAME, name)
        tracing.set_tag(IAM_ROLE_NAME_TAG_NAME, name)

        self._iam.remove_role_from_instance_profile(  # type: ignore
            InstanceProfileName=name,
            RoleName=name,
        )

    @tracing.trace("Detach all role policies")
    @check_error(DOESNT_EXIST_ERROR)
    def detach_role_policy(self, name: str, policy_arn: str):
        tracing.set_tag(IAM_ROLE_NAME_TAG_NAME, name)
        tracing.set_tag(IAM_POLICY_ARN_TAG_NAME, policy_arn)

        self._iam.detach_role_policy(  # type: ignore
            RoleName=name,
            PolicyArn=policy_arn,
        )

    def _get_tags(self) -> List[dict]:
        return [{'Key': k, 'Value': v} for k, v in self.config.aws.labels.items()]

    @tracing.trace('IAM Create User Impl')
    def create_user(self, user_name: str, path: str) -> None:
        tracing.set_tag(IAM_USER_TAG_NAME, user_name)
        try:
            response = self._iam.create_user(  # type: ignore
                UserName=user_name,
                Path=path,
                Tags=self._get_tags(),
            )
            self.logger.debug('Create IAM user %r, user_id: %r', user_name, response.get('User').get('UserId'))
        except botocore.exceptions.ClientError as error:
            if error.response['Error']['Code'] == 'EntityAlreadyExists':
                self.logger.debug('IAM user %r already exists', user_name)
            else:
                raise

    @tracing.trace('IAM Delete User Impl')
    def delete_user(self, user_name: str) -> None:
        tracing.set_tag(IAM_USER_TAG_NAME, user_name)
        try:
            self.delete_all_credentials(user_name)
            self._iam.delete_user(  # type: ignore
                UserName=user_name,
            )
            self.logger.debug('Delete IAM user %r', user_name)
        except botocore.exceptions.ClientError as error:
            if error.response['Error']['Code'] == 'NoSuchEntity':
                self.logger.debug('IAM user %r already not exists', user_name)
            else:
                raise

    def create_user_rollback(self, user_name: str) -> Callable:
        return lambda task, safe_revision: self.delete_user(user_name)

    def delete_user_rollback(self, user_name: str, path: str) -> Callable:
        return lambda task, safe_revision: self.create_user(user_name, path)

    def delete_all_credentials(self, user_name: str):
        result = self._iam.list_access_keys(UserName=user_name)  # type: ignore
        for key in result['AccessKeyMetadata']:
            access_key = key['AccessKeyId']
            Change(f'iam.user.{user_name}.access_key.{access_key}', 'delete')
            self._iam.delete_access_key(UserName=user_name, AccessKeyId=access_key)  # type: ignore

    @tracing.trace('Create access key')
    def create_access_key(self, user_name: str) -> tuple[str, str]:
        tracing.set_tag(IAM_USER_TAG_NAME, user_name)

        result = self._iam.create_access_key(UserName=user_name)
        access_key_id = result['AccessKey']['AccessKeyId']
        secret_access_key = result['AccessKey']['SecretAccessKey']
        return access_key_id, secret_access_key

    def get_user_arn(self, user_name: str) -> str:
        response = self._iam.get_user(UserName=user_name)
        return response['User']['Arn']

    @tracing.trace("Put inline policy to role")
    def put_inline_policy_to_role(self, role_name: str, policy_name: str, policy: str) -> None:
        tracing.set_tag(IAM_ROLE_NAME_TAG_NAME, role_name)
        tracing.set_tag(IAM_POLICY_NAME_TAG_NAME, policy_name)
        self._iam.put_role_policy(
            RoleName=role_name,
            PolicyName=policy_name,
            PolicyDocument=policy,
        )

    @tracing.trace("Delete inline policy from role")
    @check_error(DOESNT_EXIST_ERROR)
    def delete_inline_policy_from_role(self, role_name: str, policy_name: str) -> None:
        tracing.set_tag(IAM_ROLE_NAME_TAG_NAME, role_name)
        tracing.set_tag(IAM_POLICY_NAME_TAG_NAME, policy_name)
        self._iam.delete_role_policy(
            RoleName=role_name,
            PolicyName=policy_name,
        )


class AWSIAM(BaseProvider):
    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        self.byoa = BYOA(config, task, queue)
        self.pillar = DbaasPillar(config, task, queue)

    @cached_property
    def _client(self) -> _Client:
        if not self.config.aws_iam.enabled:
            raise IAMDisabledError
        return _Client(self.config, self.task, self.queue, self.byoa.iam_role)

    @cached_property
    def _default_account_client(self) -> _Client:
        if not self.config.aws_iam.enabled:
            raise IAMDisabledError
        return _Client(self.config, self.task, self.queue, lambda: self.config.aws.dataplane_role_arn)

    @tracing.trace("Role exists")
    def role_exists(self, role: AWSRole, assume_role_policy: str, managed_policy_arns: List[str]):
        self._client.create_role(role.name, role.path, assume_role_policy)

        self._client.create_instance_profile(role.name, role.path)
        self._client.add_role_to_instance_profile(role.name, role.name)

        if self.byoa.is_byoa():
            # Create role in default dataplane account that can be assumed only by BYOA cluster role
            self._default_account_client.create_role(
                role.name,
                role.path,
                BYOA_ASSUME_ROLE_POLICY_TEMPLATE.format(role_arn=role.arn),
            )
            # This role should have access to deb repo s3 buckets
            for arn in managed_policy_arns:
                self._default_account_client.attach_policy(role.name, arn)

            # Put to cluster role some default policies
            self._client.put_inline_policy_to_role(role.name, INLINE_POLICY_NAME, DEFAULT_BYOA_CLUSTER_ROLE_POLICY)
        else:
            for arn in managed_policy_arns:
                self._client.attach_policy(role.name, arn)

    @tracing.trace("Role absent")
    def role_absent(self, name: str, managed_policy_arns: List[str]):
        self._client.remove_role_from_instance_profile(name)
        self._client.delete_instance_profile(name)

        if self.byoa.is_byoa():
            for policy in managed_policy_arns:
                self._default_account_client.detach_role_policy(name, policy)
            self._default_account_client.delete_role(name)
            self._client.delete_inline_policy_from_role(name, INLINE_POLICY_NAME)
        else:
            for policy in managed_policy_arns:
                self._client.detach_role_policy(name, policy)

        self._client.delete_role(name)

    @tracing.trace('IAM Create User')
    def create_user(self, user_name: str, path: str) -> None:
        self.add_change(
            Change(
                f'iam.user.{path}{user_name}',
                'create',
                rollback=self._client.create_user_rollback(user_name),
            )
        )
        self._client.create_user(
            user_name=user_name,
            path=path,
        )

    @tracing.trace('IAM Delete User')
    def delete_user(self, user_name: str, path: str) -> None:
        self.add_change(
            Change(
                f'iam.user.{user_name}',
                'delete',
                rollback=self._client.delete_user_rollback(user_name, path),
            )
        )
        self._client.delete_user(
            user_name=user_name,
        )

    @tracing.trace('Get user ARN')
    def get_user_arn(self, user_name: str) -> str:
        tracing.set_tag(IAM_USER_TAG_NAME, user_name)
        return self._client.get_user_arn(user_name)

    @tracing.trace('Create IAM credentials')
    def create_access_key(self, user_name: str):
        self.add_change(Change(f'iam.user.{user_name}.credentials', 'recreate'))

        # TODO: Get/put credentials from/to some secret storage
        # instead of recreate on every retry
        self._client.delete_all_credentials(user_name)

        return self._client.create_access_key(user_name)

    @tracing.trace('Get cluster role')
    def get_cluster_role(self) -> AWSRole:
        """
        Find role in pillar.
        If found, return it.
        Otherwise, generate new one and store it in pillar.
        """
        pillar_data = self.pillar.get('cid', self.task['cid'], ROLE_PILLAR_PATH)
        if pillar_data:
            return AWSRole(**pillar_data)

        role = generate_cluster_role(self.task['cid'], self.byoa.account())
        self.pillar.exists('cid', self.task['cid'], ROLE_PILLAR_PATH, role._fields, role)
        return role


def generate_cluster_role(cid: str, account_id: str) -> AWSRole:
    name = f'mdb-dataplane-node-{cid}'
    path = DOUBLE_CLOUD_CLUSTERS_IAM_PATH
    return AWSRole(
        name=name,
        path=path,
        arn=f"arn:aws:iam::{account_id}:role{path}{name}",
        instance_profile_arn=f"arn:aws:iam::{account_id}:instance-profile{path}{name}",
    )
