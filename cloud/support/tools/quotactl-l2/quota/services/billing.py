#!/usr/bin/env python3
"""This module contains Billing class."""

from quota.base import Base
from quota.constants import SERVICES
from quota.error import FeatureNotImplemented


class Billing(Base):
    """This object represents a billing metadata for subject."""

    __DEFAULT_LIMITS__ = {}

    def __init__(self,
                 client=None,
                 request=None,
                 **kwargs):

        self.name = 'billing'
        self.client = client
        self._request = request or SERVICES[self.name]['client']
        self.endpoint = client.endpoint

    def resolve_billing_id(self, cloud_id):
        """Resolve cloud_id to billing_id."""
        url = f'{self.endpoint}/billing/v1/private/support/resolve'
        data = {
            'cloud_id': cloud_id
        }
        return self._request(self.client).post(url, json=data).get('id')

    def metadata(self, subject: str):
        """Return full info about billing account."""
        billing_id = self.resolve_billing_id(subject)
        url = f'{self.endpoint}/billing/v1/private/billingAccounts/{billing_id}/fullView'
        response = self._request(self.client).get(url)
        return BillingMetadata.de_json(response, self.client)

    def get_metrics(self, subject: str):
        raise FeatureNotImplemented('The billing service does not have a quota.')

    def update_metric(self, subject: str, metric: str, value: int):
        raise FeatureNotImplemented('The billing service does not have a quota.')

    def zeroize(self, subject: str):
        raise FeatureNotImplemented('The billing service does not have a quota.')

    def set_to_default(self, subject: str):
        raise FeatureNotImplemented('The billing service does not have a quota.')


class BillingMetadata(Base):

    def __init__(self,
                 displayStatus=None,
                 usageStatus=None,
                 balance=None,
                 billingThreshold=None,
                 personType=None,
                 **kwargs):

        self.state = displayStatus.lower() if isinstance(displayStatus, str) else displayStatus
        self.status = usageStatus
        self.type = personType
        self.balance = balance
        self.credit = billingThreshold

        super().handle_unknown_kwargs(self, **kwargs)

    @classmethod
    def de_json(cls, data: dict, client: object):
        if not data:
            return None

        data = super(BillingMetadata, cls).de_json(data, client)
        return cls(client=client, **data)
