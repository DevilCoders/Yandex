import requests


def set_auth(url, value):
    r = requests.post(url + "gauthling/setAuthenticateRequests", json={"value": value})
    assert r.status_code == 200, r.text


def set_authz(url, value):
    r = requests.post(url + "gauthling/setAuthorizeRequests", json={"value": value})
    assert r.status_code == 200, r.text


def test_set_authenticate_requests(gauthling_client, gauthling_control_url):
    # By default gauthling mock authenticates all requests
    assert gauthling_client.auth(b"token") is True

    set_auth(gauthling_control_url, False)
    assert gauthling_client.auth(b"token") is False

    set_auth(gauthling_control_url, True)
    assert gauthling_client.auth(b"token") is True


def test_set_authorize_requests(gauthling_client, gauthling_control_url):
    # By default gauthling mock authorizes all requests
    assert gauthling_client.authz(b"token", {}) is True

    set_authz(gauthling_control_url, False)
    assert gauthling_client.authz(b"token", {}) is False

    set_authz(gauthling_control_url, True)
    assert gauthling_client.authz(b"token", {}) is True


def test_state(gauthling_control_url):
    set_auth(gauthling_control_url, True)
    set_authz(gauthling_control_url, True)
    r = requests.get(gauthling_control_url + "gauthling/state")
    assert r.status_code == 200, r.text
    state = r.json()
    assert state == {"authenticate_requests": True, "authorize_requests": True}

    set_auth(gauthling_control_url, False)
    set_authz(gauthling_control_url, False)
    r = requests.get(gauthling_control_url + "gauthling/state")
    assert r.status_code == 200, r.text
    state = r.json()
    assert state == {"authenticate_requests": False, "authorize_requests": False}


def set_new_auth(url, value):
    r = requests.post(url + "access_service/setAuthenticateRequestsSubject", json=value)
    assert r.status_code == 200, r.text


def set_new_authz(url, value):
    r = requests.post(url + "access_service/setAuthorizeRequestsSubject", json=value)
    assert r.status_code == 200, r.text


def test_new_state(gauthling_control_url):
    set_new_auth(gauthling_control_url, {"id": "0000-0000-0000-0000", "user_account": {"email": "my@email.com"}})
    set_new_authz(gauthling_control_url, {"id": "0000-0000-0000-0000", "user_account": {"email": "my@email.com"}})
    r = requests.get(gauthling_control_url + "access_service/state")
    assert r.status_code == 200, r.text
    state = r.json()
    assert state == {"authenticate_requests": {"id": "0000-0000-0000-0000", "user_account": {"email": "my@email.com"}},
                     "authorize_requests": {"id": "0000-0000-0000-0000", "user_account": {"email": "my@email.com"}}}

    set_new_auth(gauthling_control_url, None)
    set_new_authz(gauthling_control_url, None)
    r = requests.get(gauthling_control_url + "access_service/state")
    assert r.status_code == 200, r.text
    state = r.json()
    assert state == {"authenticate_requests": None, "authorize_requests": None}
