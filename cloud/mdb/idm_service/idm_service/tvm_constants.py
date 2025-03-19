"""Tvm constants module"""
from tvmauth import BlackboxTvmId


class TvmConstants:
    """Tvm constants class"""

    def __init__(self):
        self.client_id = None
        self.client_secret = None
        self.env = None
        self.blackbox_client = None
        self.idm_client_id = None
        self.soc_client_id = None
        self.enabled = False

    def init_tvm(self, config):
        """Method to throw constants from file to tvm client creation"""

        self.client_id = config.get('CLIENT_ID', '')
        self.client_secret = config.get('CLIENT_SECRET', '')
        self.env = config.get('ENV', 'test')
        self.enabled = config.get('TVM_ENABLED', True)

        client_default = {'test': -1, 'prod': -1}
        idm_clients = config.get('IDM_CLIENTS', {'idm': client_default, 'soc': client_default})

        if self.env == 'test':
            self.idm_client_id = idm_clients['idm']['test']
            self.soc_client_id = idm_clients['soc']['test']
            self.blackbox_client = BlackboxTvmId.Test

        else:
            self.idm_client_id = idm_clients['idm']['prod']
            self.soc_client_id = idm_clients['soc']['prod']
            self.blackbox_client = BlackboxTvmId.Prod


TVM_CONSTANTS = TvmConstants()
