import logging

import requests
from requests.adapters import HTTPAdapter
from urllib3.util.retry import Retry


class Requester:
    def _req_retry_session(self, retries=3, backoff_factor=0.3, status_forcelist=None, session=None):
        if status_forcelist is None:
            status_forcelist = [500, 502, 504]
        session = session or requests.Session()
        retry = Retry(
            total=retries,
            read=retries,
            connect=retries,
            backoff_factor=backoff_factor,
            status_forcelist=status_forcelist,
        )
        adapter = HTTPAdapter(max_retries=retry)
        # noinspection HttpUrlsUsage
        session.mount('http://', adapter)
        session.mount('https://', adapter)
        return session

    def request(
        self,
        url,
        req_type="GET",
        verify=True,
        headers=None,
        params=None,
        data=None,
    ):
        req = None
        try:
            sess = self._req_retry_session()
            logging.info(f"request_type: {req_type}, url {url}, params {params}, data {data}")
            if req_type == "GET":
                req = sess.get(url=url, headers=headers, params=params, verify=verify)
            if req_type == "POST":
                req = sess.post(url=url, headers=headers, params=params, verify=verify, data=data)

            req.raise_for_status()
            return req.json()

        except ValueError:
            logging.warning("No valid json was provided in response")

        except Exception as err:
            logging.error(
                f"can't proceed {req_type} request for {url} with {err} {req.status_code if req is not None else ''}. "
                f"response: {req.text if req else 'unknown'}"
            )

        return {}
