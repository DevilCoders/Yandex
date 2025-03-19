"""Common helper functions"""

import contextlib
import json
import logging
import os
import uuid

import requests
import tempfile
from typing import Dict, List, Optional

from conftest import BOOTSTRAP_API_PORT

log = logging.getLogger(__name__)


@contextlib.contextmanager
def temporary_file_with_content(content: str) -> str:
    """Tempoarary file, created from content"""
    with tempfile.TemporaryDirectory() as tmp_dir:
        fname = os.path.join(tmp_dir, "file")
        with open(fname, "w") as f:
            f.write(content)
        yield fname


def assert_dicts_equal(d1: Dict, d2: Dict, d1_descr: str = "Response json", d2_descr: str = "Expected json"):
    """Assert if two dicts are equal recursively

       FIXME: assert in better format (e. g. like pytest formats different dicts"""
    d1_jsoned = json.dumps(d1, sort_keys=True, indent=4)
    d2_jsoned = json.dumps(d2, sort_keys=True, indent=4)
    assert d1_jsoned == d2_jsoned, f"\n{d1_descr}:\n{d1_jsoned}\n\n{d2_descr}:\n{d2_jsoned}"


def _bootstrap_api_req(method: str, path: str, request_headers: Optional[Dict] = None,
                       request_data: Optional[Dict] = None) -> requests.Response:
    """Perform request to bootstrap api"""
    url = "http://localhost:{}/{}".format(BOOTSTRAP_API_PORT, path)

    method = getattr(requests, method)
    resp = method(url, headers=request_headers, json=request_data)
    log.debug(resp.text)

    return resp


def _remove_ignore_path(data: Dict, ignore_path: List[str]) -> None:
    """Remove some path from dict (update <data> inplace)"""
    for idx, elem in enumerate(ignore_path):
        if elem not in data:
            break

        if idx == len(ignore_path) - 1:
            data.pop(elem)
        else:
            data = data[elem]


def bootstrap_api_req(
        method: str, path: str, expected_status: Optional[str] = "success", expected_json: Optional[Dict] = None,
        expected_data: Optional[Dict] = None, ignore_paths: Optional[List[str]] = None,
        request_headers: Optional[Dict] = None, request_data: Optional[Dict] = None) -> requests.Response:
    """Check than bootstrap api responded as expected"""
    if ignore_paths is None:
        ignore_paths = []

    rid = str(uuid.uuid4())[:8]
    if request_headers is None:
        request_headers = {}
    request_headers["X-Request-Id"] = rid

    resp = _bootstrap_api_req(method, path, request_headers=request_headers, request_data=request_data)
    resp_json = resp.json()

    if expected_status and (expected_json is None):
        assert resp_json["status"] == expected_status

    if expected_data is not None:
        for ignore_path in ignore_paths:
            _remove_ignore_path(resp_json, ignore_path.split("."))
        assert_dicts_equal(resp_json["data"], expected_data)

    if expected_json is not None:
        if "error_message" in expected_json and expected_json["error_message"] is not None:
            expected_json["error_message"] = "{}. rid={}".format(expected_json["error_message"], rid)
        for ignore_path in ignore_paths:
            _remove_ignore_path(resp_json, ignore_path.split("."))
        assert_dicts_equal(resp_json, expected_json)

    return resp
