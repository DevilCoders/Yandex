"""
Mysql replication ssh keys manipulation
"""

from functools import partial

from .pillar import DbaasPillar
from .ssh_key import SshKeyProvider


class MyReplKeyProvider(SshKeyProvider):
    """
    Mysql replication ssh keys provider
    """

    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        self.pillar = DbaasPillar(config, task, queue)

    def exists(self, cid):
        """
        Ensure that keys for mysql replication exists
        """
        self.pillar.exists(
            'cid',
            cid,
            ['data', 'mysql', 'repl_ssh_keys'],
            ['public', 'private'],
            partial(self.generate),
        )
