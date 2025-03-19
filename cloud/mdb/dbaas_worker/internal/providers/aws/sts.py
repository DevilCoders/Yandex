"""
Mostly based on https://stackoverflow.com/a/54473705
"""
from functools import partial
from typing import Any, Callable
import boto3
from botocore.credentials import (
    AssumeRoleProvider,
    AssumeRoleCredentialFetcher,
    DeferredRefreshableCredentials,
    CredentialResolver,
)
import botocore.session

from ...utils import get_absolute_now
from .byoa import BYOA


class _RamAssumeRoleProvider(AssumeRoleProvider):
    """
    Overrides default AssumeRoleProvider to not use profiles from filesystem.
    """

    def __init__(
        self,
        botocore_source_session: botocore.session.Session,
        assume_role_arn_factory: Callable,
        expiry_window_seconds: int,
        region_name: str,
    ):
        # Explicitly specify the region for a client, cause without it, we fail in dist build.
        # https://paste.yandex-team.ru/5631807
        self.__client_creator = partial(botocore_source_session.create_client, region_name=region_name)
        super().__init__(
            load_config=lambda: botocore_source_session.full_config,
            client_creator=self.__client_creator,
            cache={},
            profile_name='not-used',
        )
        self.expiry_window_seconds = expiry_window_seconds
        self.botocore_source_session = botocore_source_session
        self.assume_role_arn_factory = assume_role_arn_factory

    def load(self):
        fetcher = AssumeRoleCredentialFetcher(
            client_creator=self.__client_creator,
            source_credentials=self.botocore_source_session.get_credentials(),
            role_arn=self.assume_role_arn_factory(),
            expiry_window_seconds=self.expiry_window_seconds,
            cache=self.cache,
        )

        return DeferredRefreshableCredentials(
            method=self.METHOD,
            refresh_using=fetcher.fetch_credentials,
            time_fetcher=get_absolute_now,
        )


def get_assume_role_session(
    config: Any,
    task: Any,
    queue: Any,
    aws_access_key_id: str,
    aws_secret_access_key: str,
    region_name: str,
    expiry_window_seconds=15 * 60,
) -> boto3.Session:
    """
    Creates a new boto3 session that will operate as of another role.

    Source session must have permission to call sts:AssumeRole on the provided ARN,
    and that ARN role must have been trusted to be assumed from this account (where source_session is from).

    See https://docs.aws.amazon.com/IAM/latest/UserGuide/tutorial_cross-account-with-roles.html
    """
    byoa = BYOA(config, task, queue)
    return get_assume_role_session_with_role(
        role_factory=byoa.iam_role,
        aws_access_key_id=aws_access_key_id,
        aws_secret_access_key=aws_secret_access_key,
        region_name=region_name,
        expiry_window_seconds=expiry_window_seconds,
    )


def get_assume_role_session_with_role(
    role_factory: Callable,
    aws_access_key_id: str,
    aws_secret_access_key: str,
    region_name: str,
    expiry_window_seconds=15 * 60,
) -> boto3.Session:
    # must have .load() method to be used in CredentialsResolver.
    source_session = botocore.session.get_session()
    source_session.set_credentials(access_key=aws_access_key_id, secret_key=aws_secret_access_key)
    provider = _RamAssumeRoleProvider(
        botocore_source_session=source_session,
        assume_role_arn_factory=role_factory,
        expiry_window_seconds=expiry_window_seconds,
        region_name=region_name,
    )

    # must have .load_credentials() method to be used in register_component()
    resolver = CredentialResolver([provider])
    new_botocore_session = botocore.session.get_session()
    new_botocore_session.register_component('credential_provider', resolver)  # type: ignore
    return boto3.Session(region_name=region_name, botocore_session=new_botocore_session)
