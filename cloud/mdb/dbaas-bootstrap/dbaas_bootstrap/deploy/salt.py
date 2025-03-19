"""
Salt deploy module
"""
# pylint: disable=too-few-public-methods
from ..bootstrap import Bootstrap
from .base import CommonDeploy
from .commands import SSHCommand


@Bootstrap.deploy_handler('salt')
class SaltDeploy(CommonDeploy):
    """
    Deploy class for salt
    """
    def __init__(self, config, global_config):
        super().__init__(config, global_config)
        self.commands.append(
            SSHCommand(commands=[
                # Wait while lsyncd on salt-master copies files
                'while ! test -f /srv/conf/pillar/dbaas_pillar.py;'
                ' do sleep 1; done',
                'service salt-master restart',
                'service nginx restart',
            ]))

    def _my_salt(self):
        return self.global_config.external_salt

    def deploy(self, compute):
        """
        We need to override common compute module with our custom
        """
        # Override salt url, because we don't have any salt masters in the beginning
        old_url = compute.salt_file.base_url
        compute.salt_file.base_url = 'https://{salt}'.format(salt=self.global_config.external_salt)
        super().deploy(compute)
        compute.salt_file.base_url = old_url
