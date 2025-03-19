"""
ClickHouse backup key pillar manipulation
"""

import random
import string
from functools import partial

from ...crypto import encrypt
from ..common import BaseProvider
from ..pillar import DbaasPillar


class BackupKey(BaseProvider):
    """
    ClickHouse backup key provider
    """

    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        self.pillar = DbaasPillar(config, task, queue)

    def _generate(self):
        key = ''.join(random.SystemRandom().choice(string.ascii_letters + string.digits) for _ in range(32))

        return (encrypt(self.config, key),)

    def exists(self, subcid):
        """
        Ensure that key for ClickHouse backups exists
        """
        self.pillar.exists(
            'subcid',
            subcid,
            ['data', 'ch_backup'],
            ['encryption_key'],
            partial(self._generate),
        )
