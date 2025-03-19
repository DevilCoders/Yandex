# -*- coding: utf-8 -*-
"""
DBaaS Internal API common traits
"""

import re
from typing import Callable, List, Union

from marshmallow.exceptions import ValidationError

from . import config
from .validation import uri_validator


class InvalidInit(Exception):
    """
    Initialization error
    """


class ValidString:
    """
    Describes valid string value.
    """

    # pylint: disable=redefined-builtin
    def __init__(
        self,
        regexp: Union[str, Callable[[], str]],
        min: int,
        max: int,
        blacklist: List[str] = None,
        name: str = 'Value',
        error_messages: dict = None,
    ) -> None:
        if min < 0:
            raise InvalidInit('min must be >= 0')

        if min > max:
            raise InvalidInit('min must be <= max')

        self.min = min
        self.max = max
        self.blacklist = blacklist or []
        self._name = name
        self._regexp = regexp
        self._error_messages = {
            'invalid_length': '{name} must be between {min} and {max} characters long',
            'regexp_mismatch': '{name} \'{value}\' does not conform to naming rules',
            'blacklist_match': '{name} \'{value}\' is not allowed',
        }
        self._error_messages.update(error_messages or {})

    @property
    def regexp(self) -> str:
        """
        Validation regexp.
        """
        if isinstance(self._regexp, str):
            return self._regexp

        return self._regexp()

    def _error_message(self, name) -> str:
        err = self._error_messages[name]
        if isinstance(err, str):
            return err

        return err()

    def validate(self, value: str, extype=ValidationError, blacklist: List[str] = None) -> None:
        """
        Validates string against
        """
        if blacklist is None:
            blacklist = []

        # pylint: disable=len-as-condition
        if len(value) < self.min or len(value) > self.max:
            raise extype(self._error_message('invalid_length').format(name=self._name, min=self.min, max=self.max))
        if re.fullmatch(self.regexp, value) is None:
            raise extype(self._error_message('regexp_mismatch').format(name=self._name, value=value))
        if value in self.blacklist + blacklist:
            raise extype(self._error_message('blacklist_match').format(name=self._name, value=value))


class ClusterName(ValidString):
    """
    Describes general valid cluster name value.
    """

    # pylint: disable=redefined-builtin
    def __init__(
        self, regexp: str = '[a-zA-Z0-9_-]+', min: int = 1, max: int = 63, name: str = 'Cluster name', **kwargs
    ) -> None:
        super().__init__(regexp=regexp, min=min, max=max, name=name, **kwargs)


class ShardName(ValidString):
    """
    Describes general valid shard name value.
    """

    # pylint: disable=redefined-builtin
    def __init__(
        self, regexp: str = '[a-zA-Z0-9_-]+', min: int = 1, max: int = 63, name: str = 'Shard name', **kwargs
    ) -> None:
        super().__init__(regexp=regexp, min=min, max=max, name=name, **kwargs)


class DatabaseName(ValidString):
    """
    Describes general valid database name value.
    """

    # pylint: disable=redefined-builtin
    def __init__(
        self, regexp: str = '[a-zA-Z0-9_-]+', min: int = 1, max: int = 63, name: str = 'Database name', **kwargs
    ) -> None:
        super().__init__(regexp=regexp, min=min, max=max, name=name, **kwargs)


class UserName(ValidString):
    """
    Describes general valid user name value.
    """

    # pylint: disable=redefined-builtin
    def __init__(
        self, regexp: str = '[a-zA-Z0-9_][a-zA-Z0-9_-]*', min: int = 1, max: int = 63, name: str = 'User name', **kwargs
    ) -> None:
        super().__init__(regexp=regexp, min=min, max=max, name=name, **kwargs)


class Password(ValidString):
    """
    Describes general valid password value.
    """

    # pylint: disable=redefined-builtin
    def __init__(self, regexp: str = '.*', min: int = 8, max: int = 128, name: str = 'Password', **kwargs) -> None:
        super().__init__(regexp=regexp, min=min, max=max, name=name, **kwargs)


class ServiceAccountId(ValidString):
    """
    Describes general valid service account id.
    """

    # pylint: disable=redefined-builtin
    def __init__(
        self, regexp: str = '[a-zA-Z0-9._-]+', min: int = 1, max: int = 63, name: str = 'Service account id', **kwargs
    ) -> None:
        super().__init__(regexp=regexp, min=min, max=max, name=name, **kwargs)


class VersionPrefix(ValidString):
    """
    Describes valid semantic version prefix.
    """

    # pylint: disable=redefined-builtin
    def __init__(
        self,
        regexp: str = r'^\d+(\.\d+)?(\.\d+)?$',
        min: int = 1,
        max: int = 63,
        name: str = 'Version prefix',
        **kwargs
    ) -> None:
        super().__init__(regexp=regexp, min=min, max=max, name=name, **kwargs)


class LogGroupId(ValidString):
    """
    Describes general valid log group id.
    """

    # pylint: disable=redefined-builtin
    def __init__(
        self, regexp: str = '[a-zA-Z0-9._-]*', min: int = 0, max: int = 63, name: str = 'Log group id', **kwargs
    ) -> None:
        super().__init__(regexp=regexp, min=min, max=max, name=name, **kwargs)


class ExternalResourceURI(ValidString):
    """
    Describes valid URI reference to external resource.
    """

    # pylint: disable=redefined-builtin
    def __init__(self, name: str = 'URI', suffix_regexp='') -> None:
        super().__init__(
            regexp=lambda: config.external_uri_validation()['regexp'] + suffix_regexp,
            min=1,
            max=512,
            name=name,
            error_messages={
                'regexp_mismatch': lambda: '{name} \'{value}\' is invalid. '
                + config.external_uri_validation().get('message', ''),
            },
        )

    def validate(self, value: str, extype=ValidationError, blacklist: List[str] = None) -> None:
        super().validate(value, extype=extype, blacklist=blacklist)
        uri_validator().validate(value)
