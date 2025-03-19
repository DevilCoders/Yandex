#!/usr/bin/env python3
"""This module contains QuotaService class."""

import os
import json
import logging
from dataclasses import dataclass
from datetime import datetime

import requests
import grpc
import jwt

import quota.services as services
import time
from yandex.cloud.priv.iam.v1 import iam_token_service_pb2_grpc as iam_token_service
from yandex.cloud.priv.iam.v1 import iam_token_service_pb2 as iam_token_service_pb2


from quota.base import Base
from quota.utils.helpers import log
from quota.utils.validators import validate_service, validate_subject_id
from quota.utils.request import gRPCRequest
from quota.constants import (SERVICES, SERVICE_ALIASES,
                             IDENTITY_URL, ENVIRONMENTS, IDENTITY_URL_GRPC)
from quota.error import (TooManyParams, ServiceError, EnvError,
                         SSLError, CredentialsError, BadRequest)

logger = logging.getLogger(__name__)

@dataclass
class IamToken:
    token: str
    expires_at: datetime


class QuotaService(Base):
    """This class provides an interface for Yandex.Cloud quotas.

    Params:
      :oauth_token: str
      :iam_token: str
      :ssl: str [path to AllCAs.pem]
      :service: str [quota service name]
      :env: str [`prod` or `preprod`]
      :endpoint: str

    Static methods:
      get_iam_token - return IAM-token from OAuth-token.

    Public methods:
      metadata - return metadata for subject. Currently work only for billing service.
      get_metric - return quota metrics for subject.
      update_metric - update concrete object metric for subject.
      zeroize_metrics - set all service quota metrics to zero.
      reset_metrics_to_default - reset all service quota metrics to default values.

    """

    def __init__(self,
                 oauth_token=None,
                 iam_token=None,
                 ssl=None,
                 service=None,
                 env='prod',
                 endpoint=None,
                 **kwargs):

        __credentials__ = [cred for cred in (oauth_token, iam_token) if cred is not None]

        if len(__credentials__) > 1:
            error = 'Too many credentials received, but only one credential can be specified'
            raise TooManyParams(error)

        elif len(__credentials__) == 0:
            error = 'Credentials is empty. Please, specify iam-token or oauth-token'
            raise CredentialsError(error)

        validate_service(service)

        if env is None and env not in ENVIRONMENTS:
            raise EnvError(f'Unknown env "{env}" received. Supported: {ENVIRONMENTS}')

        if ssl is None:
            raise SSLError('Required root certificate "AllCAs.pem" not found.')
        self.token = iam_token or QuotaService.get_iam_token_grpc(self,\
                                                                  oauth_token=oauth_token,
                                                                  env=env, ssl=ssl)
        self.ssl = ssl
        self.service_name = service
        self.service = getattr(services, SERVICES[service]['object'])
        self.endpoint = endpoint or SERVICES[service]['endpoint'][env]
        self.__subject = None


        os.environ['REQUESTS_CA_BUNDLE'] = self.ssl

    @staticmethod
    def get_iam_token(oauth_token, env='prod', raw=False, ssl=''):
        """Convert OAuth-token to IAM-token."""
        from requests.packages.urllib3.exceptions import InsecureRequestWarning
        requests.packages.urllib3.disable_warnings(InsecureRequestWarning)

        if env != 'prod':
            url = f'{IDENTITY_URL[env]}/v1/tokens'
        else:
            url = f'{IDENTITY_URL[env]}/iam/v1/tokens'

        data = {'yandexPassportOauthToken': oauth_token} if env == 'prod' else {'oauthToken': oauth_token}

        r = requests.post(url, json=data, verify=False)
        r.raise_for_status()

        try:
            response = r.json()
        except json.decoder.JSONDecodeError:
            raise BadRequest(r.text)

        return response if raw else response.get('iamToken')

    @staticmethod
    def get_iam_token_grpc(account_id, key_id, private_key, env='prod', raw=False, ssl=''):
        endpoint = IDENTITY_URL_GRPC[env]
        aud = {
                'prod': 'https://iam.api.cloud.yandex.net/iam/v1/tokens',
                'preprod': 'https://iam.api.cloud-preprod.yandex.net/iam/v1/tokens'
               }

        now = int(time.time())

        payload = {
            'aud': aud[env],
            'iss': account_id,
            'iat': now,
            'exp': now + 360}

        encoded_token = jwt.encode(
            payload,
            private_key,
            algorithm='PS256',
            headers={'kid': key_id})

        with open(ssl, 'rb') as cert:
            ssl_creds = grpc.ssl_channel_credentials(cert.read())
        call_creds = grpc.access_token_call_credentials('randomshit')
        chan_creds = grpc.composite_channel_credentials(ssl_creds, call_creds)

        channel = grpc.secure_channel(endpoint, chan_creds)
        iam_stub = iam_token_service.IamTokenServiceStub(channel)

        requesst = iam_token_service_pb2.CreateIamTokenRequest(
            jwt=encoded_token
        )
        resp = iam_stub.Create(requesst)

        return IamToken(resp.iam_token, resp.expires_at.seconds) if raw else resp.iam_token


    def metadata(self, subject: str):
        """Return service metadata for subject. Currently work only for billing service."""
        if self.service_name == 'billing':
            return self.service(self).metadata(subject=subject)
        return

    def get_metrics(self, subject: str):
        """Return a subject with service quota metrics."""
        self.__subject = subject
        return self.service(self).get_metrics(subject=subject)

    def update_metric(self, subject: str, metric: str, value: int):
        """Set new limit for the specified quota metric."""
        return self.service(self).update_metric(subject=subject, metric=metric, value=value)

    def zeroize_metrics(self, subject: str):
        """Set all the service quota metrics to zero."""
        if self.service not in ('resource-manager', 'quota-calculator'):
            return self.service(self).zeroize(subject=subject)

    def reset_metrics_to_default(self, subject: str):
        """Set all the service quota metrics to default."""
        if self.service not in ('resource-manager', 'quota-calculator'):
            return self.service(self).set_to_default(subject=subject)
