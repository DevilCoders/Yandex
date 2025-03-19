"""
Mlock interaction module
"""

import os
import time
import uuid

import grpc

from cloud.mdb.mlock.api import lock_service_pb2, lock_service_pb2_grpc
from dbaas_common import retry, tracing

from ..exceptions import ExposedException
from .common import BaseProvider, Change
from .iam_jwt import IamJwt


class MlockAPIError(ExposedException):
    """
    Mlock interaction error
    """


class LockTimeout(ExposedException):
    """
    Lock timeout error
    """


class Mlock(BaseProvider):
    """
    Mlock provider
    """

    def _get_ssl_creds(self):
        cert_file = self.config.mlock.cert_file
        if not cert_file or not os.path.exists(cert_file):
            return grpc.ssl_channel_credentials()
        with open(cert_file, 'rb') as file_handler:
            certs = file_handler.read()
        return grpc.ssl_channel_credentials(root_certificates=certs)

    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        self.enabled = self.config.mlock.enabled
        if not self.enabled:
            return
        self.url = self.config.mlock.url
        self.timeout = self.config.mlock.timeout
        self._token = self.config.mlock.token
        if not self._token:
            self.iam_jwt = IamJwt(
                config,
                task,
                queue,
                service_account_id=self.config.mlock.service_account_id,
                key_id=self.config.mlock.key_id,
                private_key=self.config.mlock.private_key,
            )
        options = (('grpc.keepalive_time_ms', self.config.mlock.keepalive_time_ms),)
        if self.config.mlock.insecure:
            channel = grpc.insecure_channel(self.url, options=options)
        else:
            creds = self._get_ssl_creds()
            if self.config.mlock.server_name:
                options += (('grpc.ssl_target_name_override', self.config.mlock.server_name),)
            channel = grpc.secure_channel(self.url, creds, options)
        channel = tracing.grpc_channel_tracing_interceptor(channel)
        self.stub = lock_service_pb2_grpc.LockServiceStub(channel)

    def _get_metadata(self, request_id):
        """
        Get gRPC authorization metadata
        """
        if self._token:
            token = self._token
        else:
            token = self.iam_jwt.get_token()
        return [('authorization', f'Bearer {token}'), ('x-request-id', request_id)]

    @retry.on_exception(grpc.RpcError, factor=10, max_wait=60, max_tries=5)
    @tracing.trace('MLock Create Lock')
    def _create_lock(self, lock_id, hosts):
        """
        Create lock on hosts
        """
        tracing.set_tag('mlock.lock.id', lock_id)
        tracing.set_tag('mlock.hosts', hosts)

        request = lock_service_pb2.CreateLockRequest()
        request.holder = 'dbaas-worker'
        request.reason = f'task-{self.task["task_id"]}'
        request.id = lock_id
        request.objects.extend(hosts)

        request_id = str(uuid.uuid4())

        metadata = self._get_metadata(request_id)

        extra = self.logger.extra.copy()
        extra['request_id'] = request_id

        self.logger.logger.info('Creating lock %s on hosts %s in %s', lock_id, ', '.join(hosts), self.url, extra=extra)

        try:
            self.stub.CreateLock(request, metadata=metadata, timeout=5.0)
        except grpc.RpcError as exc:
            self.logger.logger.error('Lock creation failed: %s', repr(exc), extra=extra)
            raise

        self.logger.logger.info('Lock %s on hosts %s in %s created', lock_id, ', '.join(hosts), self.url, extra=extra)

    @retry.on_exception(grpc.RpcError, factor=10, max_wait=60, max_tries=5)
    @tracing.trace('MLock Get Lock')
    def _get_lock(self, lock_id):
        """
        Get lock status
        """
        tracing.set_tag('mlock.lock.id', lock_id)

        request = lock_service_pb2.GetLockStatusRequest()
        request.id = lock_id

        request_id = str(uuid.uuid4())

        metadata = self._get_metadata(request_id)

        extra = self.logger.extra.copy()
        extra['request_id'] = request_id

        self.logger.logger.info('Getting status of lock %s in %s', lock_id, self.url, extra=extra)

        try:
            return self.stub.GetLockStatus(request, metadata=metadata, timeout=1.0)
        except grpc.RpcError as exc:
            if hasattr(exc, 'code') and callable(exc.code):  # pylint: disable=no-member
                code = exc.code()  # pylint: disable=no-member
                if code == grpc.StatusCode.NOT_FOUND:
                    self.logger.logger.info('Lock %s not found in %s', lock_id, self.url, extra=extra)
                    return None
            self.logger.logger.error('Unable to get lock status: %s', repr(exc), extra=extra)
            raise

    @tracing.trace('MLock Release Lock')
    def _release_lock(self, lock_id):
        """
        Release lock
        """
        tracing.set_tag('mlock.lock.id', lock_id)

        request = lock_service_pb2.ReleaseLockRequest()
        request.id = lock_id

        request_id = str(uuid.uuid4())

        metadata = self._get_metadata(request_id)

        extra = self.logger.extra.copy()
        extra['request_id'] = request_id

        self.logger.logger.info('Releasing lock %s in %s', lock_id, self.url, extra=extra)

        try:
            self.stub.ReleaseLock(request, metadata=metadata, timeout=1.0)
        except grpc.RpcError as exc:
            self.logger.logger.error('Lock release failed: %s', repr(exc), extra=extra)
            raise

        self.logger.logger.info('Lock %s in %s released', lock_id, self.url, extra=extra)

    def _get_lock_id(self):
        """
        Get id for lock
        """
        return f'dbaas-worker-{self.task["cid"]}'

    @tracing.trace('MLock Cluster Lock')
    def lock_cluster(self, hosts):
        """
        Lock cluster
        """
        tracing.set_tag('mlock.enabled', self.enabled)

        if not self.enabled:
            return
        lock_id = self._get_lock_id()
        tracing.set_tag('mlock.lock.id', lock_id)
        self.add_change(
            Change(
                f'mlock.{lock_id}', 'created', rollback=lambda task, safe_revision: self.unlock_cluster(), critical=True
            )
        )
        deadline = time.monotonic() + self.timeout

        status_response = None

        with self.interruptable:
            while time.monotonic() < deadline and not getattr(status_response, 'acquired', False):
                if not status_response:
                    self._create_lock(lock_id, hosts)
                status_response = self._get_lock(lock_id)
                if getattr(status_response, 'acquired', False):
                    return
                time.sleep(1)

        if status_response:
            raise LockTimeout(
                f'Unable to get lock within {self.timeout}s timeout. ' f'Conflicts: {status_response.conflicts}.'
            )
        raise MlockAPIError(f'Unable to create lock with {self.timeout}s timeout.')

    @retry.on_exception(grpc.RpcError, factor=10, max_wait=60, max_tries=5)
    @tracing.trace('MLock Cluster Unlock')
    def unlock_cluster(self):
        """
        Unlock cluster
        """
        tracing.set_tag('mlock.enabled', self.enabled)

        if not self.enabled:
            return
        lock_id = self._get_lock_id()
        tracing.set_tag('mlock.lock.id', lock_id)
        self.add_change(Change(f'mlock.{lock_id}', 'released'))

        status_response = self._get_lock(lock_id)

        if status_response:
            self._release_lock(lock_id)
