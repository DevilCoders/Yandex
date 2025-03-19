"""
Module for provide requests to IAM Api
"""

from abc import ABC, abstractmethod


class IAM(ABC):
    """
    Abstract IAM provider
    """

    @abstractmethod
    def issue_iam_token(self, service_account_id: str):
        """
        Issue iam token for specified service_account_id
        """


class IAMError(Exception):
    """
    Base IAM Api error
    """
