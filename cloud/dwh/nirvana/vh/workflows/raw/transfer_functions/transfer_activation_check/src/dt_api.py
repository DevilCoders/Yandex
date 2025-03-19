import requests


class DtApi:
    ENDPOINT_TRANSFER = "v1/transfer"

    def __init__(self, host, auth_token) -> None:
        self._host = host
        self._auth_token = auth_token

    def status(self, transfer_id: str) -> str:
        url = f"{self._host}/{self.ENDPOINT_TRANSFER}/{transfer_id}"
        headers = {'Authorization': f'Bearer {self._auth_token}'}
        r = requests.get(url, headers=headers)

        print(r.json())

        return r.json()['status']

    def activate(self, transfer_id: str) -> bool:
        url = f"{self._host}/{self.ENDPOINT_TRANSFER}/{transfer_id}:activate"
        headers = {'Authorization': f'Bearer {self._auth_token}'}
        r = requests.post(url, headers=headers)

        print(r.json())

        if 'error' in r.json():
            return False
        return True

    def deactivate(self, transfer_id: str) -> bool:
        url = f"{self._host}/{self.ENDPOINT_TRANSFER}/{transfer_id}:deactivate"
        headers = {'Authorization': f'Bearer {self._auth_token}'}
        r = requests.post(url, headers=headers)

        print(r.json())

        if 'error' in r.json():
            return False

        return True
