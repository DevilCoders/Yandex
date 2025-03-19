from . import constants

from sandbox.common.errors import TaskFailure
from sandbox.projects.common.vcs.arc import Arc

import logging

LOGGER = logging.getLogger('arc_helper')


class ArcHelper:
    def __init__(self, arc_token):
        self._arc_client = Arc(
            arc_oauth_token=arc_token.data()[arc_token.default_key]
        )
        self._mount_point = constants.DEFAULT_VCS_MOUNT_POINT
        self._store_point = constants.DEFAULT_VCS_STORE_POINT

    @property
    def mount_point(self):
        return self._mount_point

    @property
    def store_point(self):
        return self._store_point

    @property
    def repository_object(self):
        return self._repo_obj

    def _clone_repository(self):
        LOGGER.debug('Cloning repository')

        LOGGER.debug(
            'arc mount parameters: mount_point=%s, store_point=%s',
            self.mount_point,
            self.store_point,
        )

        self._repo_obj = self._arc_client.mount_path(
            path=None,
            changeset=None,
            mount_point=self.mount_point,
            store_path=self.store_point,
            fetch_all=False,
        )

        LOGGER.debug('Repository cloned')

    def mount_repository(self):
        from library.python.retry import retry_call, RetryConf
        from sandbox.sdk2.helpers.process import subprocess

        LOGGER.debug('Preparing repository')

        retryconf = RetryConf(
            logger=LOGGER
        ).waiting(
            delay=2.,
            backoff=2.,
            jitter=1.
        ).upto(
            minutes=5.
        ).on(
            TaskFailure,
            subprocess.SubprocessError,
            subprocess.TimeoutExpired,
            Exception)
        retry_call(
            self._clone_repository,
            conf=retryconf,
        )

        LOGGER.debug('Repository has been prepared')
        LOGGER.debug(
            'Function prepare_repository returns repo: %s,'
            ' mount_point: %s, store_point: %s',
            self._repo_obj,
            self.mount_point,
            self.store_point,
        )

    def checkout(self, branch, commit):
        LOGGER.debug('Checkouting branch: %s, commit: %s', branch, commit)

        if len(commit) == 0:
            self._arc_client.checkout(self.mount_point, branch=branch)
        else:
            self._arc_client.checkout(self.mount_point, branch=commit)

    def get_status(self):
        LOGGER.debug('Getting status')
        status = self._arc_client.status(self.mount_point)
        LOGGER.debug('Status: {}'.format(status))
        return status
