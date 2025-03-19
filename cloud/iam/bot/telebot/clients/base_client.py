import threading
import requests

from requests.adapters import HTTPAdapter
from urllib3.util.retry import Retry


class BaseClient:
    def __init__(self, config):
        self._config = config
        self._thread_local = threading.local()

    @property
    def session(self):
        try:
            return self._thread_local.session
        except AttributeError:
            self._thread_local.session = self._create_session()
            return self._thread_local.session

    def _create_session(self):
        session = requests.Session()
        self._mount_session(session)
        return session

    def _mount_session(self, session):
        session.mount(self._config['api'], HTTPAdapter(max_retries=Retry(total=self._config.get('retries'))))
