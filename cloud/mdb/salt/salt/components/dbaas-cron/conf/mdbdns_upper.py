"""
MDB DNS updater

Update primary host fqdn in cluster in mdb-dns-server
"""
import json
import socket
import time
from base64 import b64encode
from logging import Formatter, getLogger
from logging.handlers import RotatingFileHandler

import requests
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import padding, utils

UPPER = None
DBAAS_CONF_PATH = '/etc/dbaas.conf'
REQUIRED_PARAMS = ['base_url', 'cid', 'fqdn', 'ca_path', 'cid_key_path']
DEFAULT_SEND_TIMEOUT_MSEC = 3000
RELOAD_TIMEOUT = 0.2


def init_logger(name, log_file, rotate_size=10485760, backup_count=1, log_level='INFO'):
    """
    Init dns upper logger
    """
    logger = getLogger(name)
    handler = RotatingFileHandler(
        log_file, maxBytes=rotate_size, backupCount=backup_count)
    handler.setFormatter(Formatter("%(asctime)s;%(levelname)s;%(message)s"))
    logger.addHandler(handler)
    logger.setLevel(log_level)
    return logger


class MDBDNSUpper:
    def __init__(self, logger, params):
        self._logger = logger
        self._logger.info("initialization...")

        missed_params = [param for param in REQUIRED_PARAMS if param not in params]
        if missed_params:
            raise Exception('missed following params in config: %s' % ', '.join(missed_params))

        self._send_timeout = float(params.get('send_timeout_ms', DEFAULT_SEND_TIMEOUT_MSEC)) / 1000
        self._base_url = params['base_url']
        self._fqdn = params['fqdn']
        self._cid = params['cid']
        self._ca_path = params['ca_path']
        self._session = requests.Session()
        adapter = requests.adapters.HTTPAdapter(pool_connections=1, pool_maxsize=1)
        self._session.mount(self._base_url, adapter)
        self._session.verify = self._ca_path
        self._cluster_key = None

        cid_key_path = params['cid_key_path']
        fqdn = socket.getfqdn()
        if fqdn != self._fqdn:
            raise Exception('Host name: "%s", not same that in config: "%s"' % (fqdn, self._fqdn))

        try:
            with open(cid_key_path, 'rb') as key_file:
                self._cluster_key = serialization.load_pem_private_key(
                    key_file.read(), password=None, backend=default_backend())
        except IOError as exc:
            raise Exception('cluster key file not found: %s, io error: %s' % (cid_key_path, exc))
        except Exception as exc:
            self._logger.error('while loading cluster private key: %s, error: %s', cid_key_path, repr(exc))
            raise exc

        if self._cluster_key is None:
            raise Exception('failed to load cluster key from "%s"' % cid_key_path)

        self._logger.info('loaded cluster key from "%s"', cid_key_path)

    def get_state(self):
        raise NotImplementedError()

    def get_dbaas_conf(self):
        """
        Get cluster configuraiton file
        """
        path = DBAAS_CONF_PATH
        try:
            with open(path) as conf:
                return json.load(conf)
        except Exception as exc:
            self._logger.error('failed to load dbaas configuration file "%s", error: %s', path, repr(exc))
            raise exc

    def sign_data(self, data):
        chosen_hash = hashes.SHA512()
        hasher = hashes.Hash(chosen_hash, default_backend())
        hasher.update(data)
        digest = hasher.finalize()

        return self._cluster_key.sign(
            digest,
            padding.PSS(
                mgf=padding.MGF1(hashes.SHA512()),
                salt_length=padding.PSS.MAX_LENGTH),
            utils.Prehashed(chosen_hash))

    def update_dns(self, secondary=None, shardid=None):
        message = {
            "timestamp": int(time.time()),
            "primarydns": {
                "cid": self._cid,
                "primaryfqdn": self._fqdn
            }
        }
        if shardid:
            message["primarydns"]["sid"] = shardid
        if secondary:
            message["primarydns"]["secondaryfqdn"] = secondary

        data = str.encode(json.dumps(message))
        sign = self.sign_data(data)

        self._logger.info('sending update to mdb-dns: {message}'.format(message=data.decode()))
        response = self._session.put(
            "%s/v1/dns" % self._base_url,
            data=data,
            headers={
                'Content-Type': 'application/json',
                'X-Signature': b64encode(sign),
            },
            timeout=self._send_timeout,
        )

        if response.status_code != 200 and response.status_code != 202 and response.status_code != 204:
            self._logger.error('send to mdb-dns failed ({code}): {response}'.format(
                response=response.text, code=response.status_code))
        else:
            self._logger.info('send to mdb-dns successful')

    def choice_best_secondary(self, state):
        raise NotImplementedError()

    def check_and_up(self):
        self._logger.debug("checking...")
        dbc = self.get_dbaas_conf()

        cid = dbc.get("cluster_id")
        if not cid or cid != self._cid:
            self._logger.error("invalid cid in dbaas conf")
            return

        state = self.get_state()
        if not state:
            self._logger.warning("invalid or unfresh state")
            return

        if state.get("role") != "master":
            self._logger.info("not a master host")
            return

        secondary = self.choice_best_secondary(state)
        self.update_dns(secondary)


def mdbdns_upper(log_file, rotate_size, params, backup_count=1):
    """
    Run mdb DNS update
    """
    global UPPER
    if not UPPER:
        log = init_logger(__name__, log_file, rotate_size, backup_count)
        log.info("Initialization MDB DNS upper")
        try:
            UPPER = MDBDNSUpper(log, params)
        except Exception as exc:
            log.error("failed to init MDB DNS upper failed by exception: %s", repr(exc))
            raise exc

    try:
        UPPER.check_and_up()
    except Exception as exc:
        UPPER._logger.error("mdbdns upper failed by exception: %s", repr(exc))
        raise exc
