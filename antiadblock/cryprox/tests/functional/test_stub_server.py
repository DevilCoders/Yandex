import os
import io

import pytest
import requests
from urlparse import urljoin, parse_qs


@pytest.mark.parametrize('path, expected_code, rel_path', [
    ('/', 200, 'index.html'),
    ('/index.html', 200, 'index.html'),
    ('/crypted/json.html', 200, 'crypted/json.html'),
    ])
def test_default_handler(stub_server, path, rel_path, expected_code, content_path):
    expected_text = io.open(os.path.join(content_path, rel_path), 'r', encoding='utf-8').read()
    response = requests.get(urljoin(stub_server.url, path))
    assert response.text.encode('ascii', 'ignore') == expected_text.encode('ascii', 'ignore')
    assert response.status_code == expected_code


def test_default_handler_not_existent_path(stub_server):
    expected_text = 'Error. Not found.'
    response = requests.get(urljoin(stub_server.url, "/not_exists"))
    assert response.text.encode('ascii', 'ignore') == expected_text.encode('ascii', 'ignore')
    assert response.status_code == 404


def path_handler(**request):
    if request.get('path', None) == '/path1':
        return {'text': 'chosen path1', 'code': 200}
    elif request.get('path', None) == '/path2':
        return {'text': 'chosen path2', 'code': 300}
    return {}


def test_user_handler_index_page(stub_server, content_path):
    stub_server.set_handler(path_handler)
    expected_text = io.open(os.path.join(content_path, "index.html"), 'r', encoding='utf-8').read()
    response = requests.get(urljoin(stub_server.url, '/', content_path, ))
    assert response.text.encode('ascii', 'ignore') == expected_text.encode('ascii', 'ignore')
    assert response.status_code == 200


@pytest.mark.parametrize('path, expected_code, expected_text', [
    ('/path1', 200, 'chosen path1'),
    ('/path2', 300, 'chosen path2'),
    ('/not_exists', 404, 'Error. Not found.')])
def test_user_handler_custom_path(stub_server, path, expected_code, expected_text):
    stub_server.set_handler(path_handler)
    response = requests.get(urljoin(stub_server.url, path))
    assert response.text.encode('ascii', 'ignore') == expected_text.encode('ascii', 'ignore')
    assert response.status_code == expected_code


def headers_handler(**request):
    if request['headers'].get('HTTP_H', None) == '1':
        return {'text': 'chosen header 1', 'code': 200}
    elif request['headers'].get('HTTP_H', None) == '2':
        return {'text': 'chosen header 2', 'code': 300}
    elif request['headers'].get('HTTP_H', None) == 'default':
        return {}
    elif request['headers'].get('HTTP_H', None) == 'error':
        return {'text': 'fail', 'code': 400}


def test_user_handler_headers_index_page(stub_server, content_path):
    stub_server.set_handler(headers_handler)
    response = requests.get(stub_server.url, headers={'h': 'default'})
    expected_text = io.open(os.path.join(content_path, "index.html"), 'r', encoding='utf-8').read()
    assert response.text.encode('ascii', 'ignore') == expected_text.encode('ascii', 'ignore')
    assert response.status_code == 200


@pytest.mark.parametrize('headers, expected_code, expected_text', [
    ({'h': '1'}, 200, 'chosen header 1'),
    ({'h': '2'}, 300, 'chosen header 2'),
    ({'h': 'error'}, 400, 'fail')])
def test_user_handler_headers(stub_server, headers, expected_code, expected_text):
    stub_server.set_handler(headers_handler)
    response = requests.get(stub_server.url, headers=headers)
    assert response.text.encode('ascii', 'ignore') == expected_text.encode('ascii', 'ignore')
    assert response.status_code == expected_code


def query_handler(**request):
    query = parse_qs(request.get('query', ''))
    if 'name' not in query:
        return {}
    if query['name'][0] == 'errorPage':
        return {'text': 'error', 'code': 300}
    elif 'color' not in query:
        return {}
    if query['name'][0] == 'carrot' and query['color'][0] == 'red':
        return {'text': 'red carrot', 'code': 200}
    elif query['name'][0] == 'ferret' and query['color'][0] == 'purple':
        return {'text': 'first', 'code': 200}


@pytest.mark.parametrize('query, expected_code, expected_text', [
    ('name=ferret&color=purple', 200, 'first'),
    ('name=carrot&color=red', 200, 'red carrot'),
    ('name=errorPage', 300, 'error')])
def test_user_handler_query(stub_server, query, expected_code, expected_text):
    stub_server.set_handler(query_handler)
    response = requests.get(stub_server.url + '?' + query)
    assert response.text == expected_text
    assert response.status_code == expected_code


def body_handler(**request):
    body = parse_qs(request.get('body', ''))
    if 'param1' not in body:
        return {}
    if body['param1'][0] == 'value1' and body['param2'][0] == 'value2':
        return {'text': 'first', 'code': 200}
    if body['param1'][0] == 'value1' and body['param2'][0] == 'value3':
        return {'text': 'second', 'code': 300}


@pytest.mark.parametrize('body,expected_code,expected_text', [
    ({'param1': 'value1', 'param2': 'value2'}, 200, 'first'),
    ({'param1': 'value1', 'param2': 'value3'}, 300, 'second'),
    ('param1=value1&param2=value2', 200, 'first'),
    ('param1=value1&param2=value3', 300, 'second'),
])
def test_user_handler_body(stub_server, body, expected_code, expected_text):
    stub_server.set_handler(body_handler)
    response = requests.post(stub_server.url, data=body)
    assert response.text == expected_text
    assert response.status_code == expected_code
