# coding=utf-8
import pytest

from antiadblock.configs_api.lib.service_api import api
from conftest import CryproxClientMock


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize('path, qargs, expected_code, expected_answer',
                         [('service/{service_id}/utils/', {'url': 'test'}, 404, 'NotFound'),
                          ('service/{service_id}/utils/decrypt_url', {}, 400, u'Ошибка валидации'),
                          ('service/{service_id}/utils/decrypt_url', {'url': ''}, 400, u'Ошибка валидации'),
                          ])
def test_wrong_requests(api, session, service, path, qargs, expected_code, expected_answer):

    response = session.post(api[path.format(service_id=service['id'])], json=qargs)
    response_text = response.json()

    assert response_text['message'] == expected_answer
    assert response.status_code == expected_code


@pytest.mark.usefixtures("transactional")
@pytest.mark.parametrize('crypted_url, expected_code, expected_decrypted_link, expected_response_msg',
                         [(k, 200, v, '') for k, v in CryproxClientMock.crypted_links.items()]
                         + [(k, 404, v, api.FAILED_TO_DECRYPT_MSG) for k, v in CryproxClientMock.not_crypted_links.items()]
                         + [(CryproxClientMock.forbidden_link, 403, '', api.FORBIDDEN_URL_DECRYPT_MSG)])
def test_decrypt_url(api, session, service, crypted_url, expected_code, expected_decrypted_link, expected_response_msg):

    response = session.post(api['service'][service['id']]['utils']['decrypt_url'], json={'url': crypted_url})
    response_text = response.json()

    assert response.status_code == expected_code
    if expected_code == 200:
        assert response_text['decrypted_url'] == expected_decrypted_link
    else:
        assert response_text['message'] == expected_response_msg
