import requests

from common.util import urllib3_force_ipv4


class CloudMetadataClient:
    METADATA_HOSTNAME = "169.254.169.254"
    REQUEST_TIMEOUT = 10  # seconds

    def get_metadata_value(self, parameter: str):
        """
        Requests metadata value. See available parameters in the documentation:
        https://cloud.yandex.ru/docs/compute/operations/vm-info/get-info#gce-metadata
        """
        with urllib3_force_ipv4():
            r = requests.get(
                f"http://{self.METADATA_HOSTNAME}/computeMetadata/v1/instance/{parameter}", headers={"Metadata-Flavor": "Google"},
                timeout=self.REQUEST_TIMEOUT,
            )
            r.raise_for_status()
            return r.text
