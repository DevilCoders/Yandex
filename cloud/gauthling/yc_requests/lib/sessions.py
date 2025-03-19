# coding: utf-8

from requests import Session as _Session

from .signing import RequestSigner, YcRequest


def merged_session_env(session, url,
                       timeout=None,
                       allow_redirects=True,
                       proxies=None,
                       stream=None,
                       verify=None,
                       cert=None):
    proxies = proxies or {}

    merged = {
        "timeout": timeout,
        "allow_redirects": allow_redirects,
    }
    merged.update(session.merge_environment_settings(url, proxies, stream, verify, cert))

    return merged


class Session(_Session):
    def __init__(self, service_name, credentials):
        self.signer = RequestSigner(service_name, credentials)
        super(Session, self).__init__()

    def request(self, method, url,
                params=None,
                data=None,
                headers=None,
                cookies=None,
                files=None,
                auth=None,
                timeout=None,
                allow_redirects=True,
                proxies=None,
                hooks=None,
                stream=None,
                verify=None,
                cert=None,
                json=None):
        # Create the Request.
        req = YcRequest(
            method=method.upper(),
            url=url,
            headers=headers,
            files=files,
            data=data or {},
            json=json,
            params=params or {},
            auth=auth,
            cookies=cookies,
            hooks=hooks,
        )

        # Sign the Request
        self.signer.sign(req)

        prep = self.prepare_request(req)

        send_kwargs = merged_session_env(self, prep.url,
                                         timeout=timeout,
                                         allow_redirects=allow_redirects,
                                         proxies=proxies,
                                         stream=stream,
                                         verify=verify,
                                         cert=cert)
        return self.send(prep, **send_kwargs)


def session(service_name, credentials):
    return Session(service_name, credentials)
