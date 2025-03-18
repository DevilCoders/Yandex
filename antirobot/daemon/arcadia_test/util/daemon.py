import urllib.request


class NoRedirection(urllib.request.HTTPErrorProcessor):
    def http_response(self, request, response):
        return response

    https_response = http_response


def AddDaemonHost(req, host):
    if isinstance(req, str):
        assert len(req) > 0
        # urllib.request.Request raises an exception if the string given doesn't
        # contain scheme and host so we have to manually add it before creating
        # an instance of urllib.request.Request
        if req[0] == '/':
            req = "http://" + host + req
        req = urllib.request.Request(req)

    assert isinstance(req, urllib.request.Request)

    assert not req.has_header("Host") or req.get_host() == host
    req.add_header("Host", host)

    return req
