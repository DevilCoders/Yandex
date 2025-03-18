# coding=utf-8
import os
from urlparse import urljoin

import requests
import yatest

from antiadblock.cryprox.cryprox.config.system import DETECT_LIB_HOST, DETECT_LIB_PATHS, NGINX_SERVICE_ID_HEADER
from antiadblock.cryprox.cryprox.common.resource_utils import url_to_filename

PERSISTENT_VOLUME_PATH = yatest.common.output_path("perm")


def handler(**_):
    return {'text': 'fail', 'code': 500}


def test_get_detect_from_cache(stub_server, cryprox_worker_url, restore_stub):
    for lib_path in DETECT_LIB_PATHS:
        restore_stub()
        script = requests.get(urljoin(stub_server.url, lib_path)).text
        # проверяем содержимое файла с кешом (создается сервисным процессом при запуске)
        with open(os.path.join(PERSISTENT_VOLUME_PATH, url_to_filename(lib_path)), mode='rb') as f:
            cached_script = f.read()
        assert cached_script == script

        # админка отвечает 500, проверяем что скрипт берется из файла
        stub_server.set_handler(handler)
        new_script = 'script: ' + lib_path
        with open(os.path.join(PERSISTENT_VOLUME_PATH, url_to_filename(lib_path)), mode='wb') as f:
            f.write(new_script)
        url = urljoin(cryprox_worker_url, lib_path) + "?pid=test_local"
        response = requests.get(url, headers={'host': DETECT_LIB_HOST})
        assert response.headers.get(NGINX_SERVICE_ID_HEADER) == 'test_local'
        assert response.status_code == 200
        assert response.text == new_script
