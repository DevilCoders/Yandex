#!/usr/bin/env python3
"""This module contains QuotaService class."""

import os
import json
import requests

import app.quota.services as services

from app.quota.base import Base
from app.quota.utils.validators import validate_service
from app.quota.constants import (SERVICES, IDENTITY_PREPROD_URL,
                                 IDENTITY_PROD_URL, ENVIRONMENTS)
from app.quota.error import (TooManyParams, EnvError, SSLError,
                             CredentialsError, BadRequest)

# logger = logging.getLogger(__name__)


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

        self.token = iam_token or QuotaService.get_iam_token(oauth_token, env=env)
        self.ssl = ssl
        self.service_name = service
        self.service = getattr(services, SERVICES[service]['object'])
        self.endpoint = endpoint or SERVICES[service]['endpoint'][env]
        self.__subject = None

        os.environ['REQUESTS_CA_BUNDLE'] = './allCA.crt'

    @staticmethod
    def get_iam_token(oauth_token, env='prod', raw=False):
        """Convert OAuth-token to IAM-token."""
        from requests.packages.urllib3.exceptions import InsecureRequestWarning
        requests.packages.urllib3.disable_warnings(InsecureRequestWarning)

        url = f'{IDENTITY_PROD_URL}/iam/v1/tokens' if env == 'prod' else f'{IDENTITY_PREPROD_URL}/v1/tokens'
        data = {'yandexPassportOauthToken': oauth_token} if env == 'prod' else {'oauthToken': oauth_token}

        r = requests.post(url, json=data, verify=False)
        r.raise_for_status()

        try:
            response = r.json()
        except json.decoder.JSONDecodeError:
            raise BadRequest(r.text)

        return response if raw else response.get('iamToken')

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
