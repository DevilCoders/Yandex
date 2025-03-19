import os
import time

import requests


class Test_connection:
    def test_ping(self):
        port = os.environ.get('DBAAS_INTERNAL_API_RECIPE_PORT')
        deadline = time.time() + 30
        while time.time() < deadline:
            try:
                resp = requests.get('http://localhost:{port}/ping'.format(port=port))
                resp.raise_for_status()
                return
            except Exception:
                time.sleep(0.1)
