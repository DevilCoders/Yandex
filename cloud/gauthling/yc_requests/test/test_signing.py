# coding: utf-8

import pytest
from yc_requests.signing import RequestSigner, YcRequest

from yc_requests.credentials import YandexCloudCredentials

SERVICE_NAME = "compute"
TOKEN = "CgcIARUCAAAAEoAEsZJ2DdV3-DiIDcabTeKnsNnJOw18OBgAxPaW6DcJdMZqCuIit5CCEC8UbME8NzIbTTkk1dMkhXwaVB2TicWvHK9s" \
        "0WuP95txKo-W6PzrZ-U87-0t1XXKY7bXPD1Q4hyMpI16cielSHVany5z-xMSv2zKs7XUhJeVF3x5tDZFCF8pkzyvhRrccJoFkAKIL1vC" \
        "l9vvS48gcd8dj5Q7nrahEJcHLq3UdAtJOkaHNTHPH8pN9muFv4DPwvGKtBrtd1Yto_hI12jUJL0IIpC7cJ5ssGtvqLD1W29eZGhsISUV" \
        "cpigNUrjNOmjlDYsHiLMnn4ev-o4NCVFEOyg15wa48XrP-CDRhY1pFjpLPQTu69KNSHSLAFFvWI7HkKvAJtQdSEmEvHIWCejMeIGszzu" \
        "uPg1QaLMIUdP5CtEZuTO8-8YL5T6DgwVvMF3n_8D-NOJ_aL_MC1oZoW2k8lOkoVeAvTAVkY2d4qIKZ-FnWGm4H2Yr-6sg-76eBxhEQPT" \
        "-rVCdvkc93S9-9R58Gn3IUGr4M2L5SXDbQpPPkZmdS8y3UVKfOksg2Je-s_2UgxEuJijtrQxpzROTSbmK4xjmZCwKe3YB-2tvD1ntNVI" \
        "dV0yYp5Jfcarw9S-2U5_eQMi1PcNU1YvG6E6BK8QWTWAymeGNS0rvv5j4lhC1b9NuqYgdAkFtm0akAEKJGJiMzQyNGYwLTFiYmMtNGU0" \
        "MC04ZmRiLTA0MTI3MDNlYWI1NBDLmrLIBRjLg8LIBSJDChN5cDoxMTMwMDAwMDAwNTI2NDM2EgppYmVsb25vZ292IiBpYmVsb25vZ292" \
        "QHljLXRlc3QueWFjb25uZWN0LmNvbSoOCgM5NTYSB3ljLXRlc3RKBwgBFQIAAAAgnwU"


@pytest.fixture(scope="module", autouse=True)
def signer():
    secret_key = "YmjpWcJ7_3C7GjDZdfIMfcItADq23EF_MPhc6dKcFvw"
    credentials = YandexCloudCredentials(token=TOKEN, secret_key=secret_key)
    return RequestSigner(SERVICE_NAME, credentials)


def request(*args, **kwargs):
    req = YcRequest(*args, **kwargs)
    # Mock request time to check signature
    req.timestamp = "20170506T123224Z"
    return req


def test_sign_request_with_headers(signer):
    headers = {
        "TestHeader-1": "foo",
        "TestHeader-2": "bar",
    }
    req = request("GET", "https://api.yandex.cloud", headers=headers.copy())
    signer.sign(req)

    for h_key in headers:
        assert h_key in req.headers and h_key.lower() in req.headers["X-YaCloud-SignedHeaders"]
    assert req.headers["X-YaCloud-SubjectToken"] == TOKEN
    assert req.headers["X-YaCloud-SignKeyService"] == SERVICE_NAME
    assert req.headers["X-YaCloud-SignMethod"] == "YC_SET1_HMAC_SHA256"
    assert req.headers["X-YaCloud-Signature"] == "54e151d835c0082354486ac1febbe0621387ddf24600b1903d904cd9738c8aa8"


def test_sign_request_with_ignored_headers(signer):
    headers = {
        "User-Agent": "Yandex.Horse/1.6.41",
        "Expect": "the Spanish Inquisition",
    }
    req1 = request("GET", "https://api.yandex.cloud", headers=headers)
    signer.sign(req1)
    req2 = request("GET", "https://api.yandex.cloud", headers=dict())
    signer.sign(req2)
    assert req1.headers["X-YaCloud-Signature"] == req2.headers["X-YaCloud-Signature"]


def test_sign_request_with_query_string(signer):
    qs = "qs_item1=foo&qs_item1=bar"
    req = request("GET", "https://api.yandex.cloud?" + qs)
    signer.sign(req)
    assert req.headers["X-YaCloud-Signature"] == "f86af970f8a5f8d3c61dd9e7e8d4d99a07b1460875d8fa0860711568d86fdc1e"


def test_sign_request_with_query_string_integer_value(signer):
    # Regression test for https://st.yandex-team.ru/CLOUD-5152
    req = request("GET", "https://api.yandex.cloud", params={"test": 1})
    signer.sign(req)
    assert req.headers["X-YaCloud-Signature"] == "3a076b1709550330b466468b011f403c8f72719d91a163fe17c0cd82d1d45505"


def test_sign_request_with_query_string_binary_value(signer):
    # Regression test for https://st.yandex-team.ru/CLOUD-5152
    req = request("GET", "https://api.yandex.cloud", params={"test": b'q'})
    signer.sign(req)
    assert req.headers["X-YaCloud-Signature"] == "da68e01c8a18ef5dbd6c60bee15eff58167b2901364d08cde8ca4ee3f8ac0712"


def test_sign_request_with_query_string_unicode_value(signer):
    # Regression test for https://st.yandex-team.ru/CLOUD-5152
    req = request("GET", "https://api.yandex.cloud", params={"test": 'бдыщь'})
    signer.sign(req)
    assert req.headers["X-YaCloud-Signature"] == "129f24fccdf361e5ecc9c2d12db9d2235f3b851bd75fca8ebb4adfc3171e4358"


def test_sign_request_with_payload(signer):
    payload = b"payloadbytes"
    req = request("POST", "https://api.yandex.cloud", data=payload)
    canonical_req = req.make_canonical_request()
    assert canonical_req.payload_checksum == "8c82ccbd965a5f5c361062235aecef01ccc79d7b91b6ecb3e8d321d0f39e954a"
    signer.sign(req)
    assert req.headers["X-YaCloud-Signature"] == "ee74198897e0e64a98179997da6145c497d48cc68ae4476f4267bc53d2593bea"


def test_sign_request_with_json_payload(signer):
    json_payload = {"payload": "horse"}
    req = request("PUT", "https://api.yandex.cloud", json=json_payload)
    canonical_req = req.make_canonical_request()
    assert canonical_req.payload_checksum == "f07b9e1a8be06f84456050f0556622623cc1fb4db37d36ee946c54aa96e4e8ec"
    signer.sign(req)
    assert req.headers["X-YaCloud-Signature"] == "dcb5b07e1dc2bb8703a5d9a2e66ff5aae8c6bf2206b5cb31778ed1de86a66fe3"
