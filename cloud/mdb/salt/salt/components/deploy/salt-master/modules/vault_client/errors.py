# -*- coding: utf-8 -*-

import sys

from paramiko.ssh_exception import (
    PasswordRequiredException,
    SSHException,
)


def wrap_unknown_errors(function_to_decorate):
    def wrapper(*args, **kwargs):
        try:
            return function_to_decorate(*args, **kwargs)
        except BaseError:
            raise
        except Exception as e:
            class _ClientWrappedError(ClientWrappedError, e.__class__):
                def __init__(self, *args, **kwargs):
                    ClientWrappedError.__init__(self, *args, **kwargs)

            raise _ClientWrappedError(e, exc_info=sys.exc_info())
    return wrapper


class BaseError(Exception):
    def __init__(self, **kwargs):
        """
        :param kwargs: поля, которые будут записаны в логи и переданы в ответ на запрос
        """
        self.kwargs = kwargs

    def __str__(self):
        return str(self.kwargs or '')


class ClientError(BaseError):
    pass


class ClientRsaKeyRequiredPassword(BaseError, PasswordRequiredException):
    def __init__(self, **kwargs):
        super(ClientRsaKeyRequiredPassword, self).__init__(
            code=u'error',
            message=u'The private key is encrypted and required password. Use an unencrypted key or add a key '
                    u'to the SSH Agent via the ssh-add command',
            **kwargs
        )


class ClientInvalidRsaPrivateKey(BaseError, SSHException):
    def __init__(self, **kwargs):
        super(ClientInvalidRsaPrivateKey, self).__init__(
            code=u'error',
            message=u'The private key is invalid',
            **kwargs
        )


class ClientRsaKeysNotFound(ClientError):
    def __init__(self, message=None, **kwargs):
        super(ClientRsaKeysNotFound, self).__init__(
            code=u'error',
            message=message or u'RSA keys not found',
            **kwargs
        )


class ClientSSHAgentError(ClientError, SSHException):
    def __init__(self, message=None, **kwargs):
        super(ClientSSHAgentError, self).__init__(
            message=message or u'SSH agent failed',
            **kwargs
        )


class ClientNoKeysInSSHAgent(ClientRsaKeysNotFound):
    def __init__(self, **kwargs):
        super(ClientNoKeysInSSHAgent, self).__init__(
            message=u'No keys in the SSH Agent. Check output of \'ssh-add -l\' command',
            **kwargs
        )


class ClientInvalidRsaKeyNumber(ClientError):
    def __init__(self, key_num=None, **kwargs):
        super(ClientInvalidRsaKeyNumber, self).__init__(
            code=u'error',
            message=u'RSA key number is out of range' + (' ({})'.format(key_num) if key_num is not None else ''),
            **kwargs
        )


class ClientUnknownKeyHashType(ClientError):
    def __init__(self, key_hash=None, **kwargs):
        super(ClientUnknownKeyHashType, self).__init__(
            code=u'error',
            message=u'Unknown RSA key hash type' + (' ({})'.format(key_hash) if key_hash is not None else ''),
            **kwargs
        )


class ClientRsaKeyHashNotFound(ClientError):
    def __init__(self, key_hash=None, **kwargs):
        super(ClientRsaKeyHashNotFound, self).__init__(
            code=u'error',
            message=u'RSA key with such hash is not found in the agent' + (' ({})'.format(key_hash) if key_hash is not None else ''),
            **kwargs
        )


class ClientUnknownRSAAuthType(ClientError):
    pass


class ClientWrappedError(ClientError):
    def __init__(self, base_error, exc_info=None, **kwargs):
        super(ClientWrappedError, self).__init__(message=str(base_error) or repr(base_error), **kwargs)
        self.base_error = base_error
        self.exc_info = exc_info
