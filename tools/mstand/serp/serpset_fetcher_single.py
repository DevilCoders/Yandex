import logging
import os
import time
from typing import Optional

import yaqutils.misc_helpers as umisc
import yaqutils.json_helpers as ujson
import yaqutils.requests_helpers as urequests
from serp import SerpFetchParams  # noqa
from serp import RawSerpDataStorage  # noqa
from yaqutils import MainUtilsException


def fetch_serpset_meta_info(serpset_id: str) -> Optional[dict]:
    api_url = "https://metrics.yandex-team.ru/services/api/serpset/info"
    params = {"id": serpset_id}
    response = urequests.retry_request(method="GET", url=api_url, times=2, params=params, verify=False)
    try:
        serpset_meta = response.json()
    except Exception as exc:
        logging.error("Cannot get serpset metainfo: %s", str(exc))
        serpset_meta = None
    return serpset_meta


def download_serpset(serpset_id: str, file_name: str, fetch_params: SerpFetchParams, meta_file_name: Optional[str] = None):
    api_url = "https://{}/api/json/{}".format(fetch_params.metrics_server, serpset_id)

    params = fetch_params.get_url_params()
    headers = fetch_params.get_headers()

    download_start_time = time.time()
    temp_download_file = file_name + ".tmp"
    urequests.download(api_url, temp_download_file, params=params, headers=headers)
    umisc.log_elapsed(download_start_time, "raw serpset %s fetched from metrics", serpset_id)

    check_serpset_size(serpset_id, temp_download_file)

    # atomic rename to avoid partially downloaded files
    os.rename(temp_download_file, file_name)
    logging.info("Raw serpset %s saved to %s", serpset_id, file_name)

    if meta_file_name:
        serpset_meta = fetch_serpset_meta_info(serpset_id)
        logging.info("Serpset %s meta-info saved to %s", serpset_id, meta_file_name)
        ujson.dump_to_file(serpset_meta, meta_file_name)


def check_serpset_size(serpset_id, file_name):
    # https://st.yandex-team.ru/MSTAND-611
    serpset_file_size = os.path.getsize(file_name)
    size_mb = float(serpset_file_size) / (1024 ** 2)
    logging.info("Downloaded serpset %s file size (compressed): %.2f Mb", serpset_id, size_mb)
    # https://st.yandex-team.ru/MSTAND-866
    if serpset_file_size < 1024:
        logging.warning("Too small serpset file, size = %s", serpset_file_size)
        raise Exception("Downloaded serpset seems to be too small: {} bytes".format(serpset_file_size))


def handle_fetch_error(serpset_id, fetch_params, exc_message):
    logging.error("Cannot fetch serpset %s: %s", serpset_id, exc_message)
    if fetch_params.skip_broken:
        logging.warning("Skipping serpset %s", serpset_id)
    else:
        raise MainUtilsException("Serpset {} download failed: {}".format(serpset_id, exc_message))


def fetch_serpset(serpset_id: str, raw_serp_storage: RawSerpDataStorage, fetch_params: SerpFetchParams) -> bool:
    serpset_file_name = raw_serp_storage.raw_gz_serpset_by_id(serpset_id)
    meta_file_name = raw_serp_storage.serpset_meta_by_id(serpset_id)

    logging.info('Fetching serpset %s from metrics as json file.', serpset_id)
    attempts = fetch_params.retry_count
    current_retry_timeout = fetch_params.retry_timeout
    max_retry_timeout = 600.0
    for try_num in range(attempts + 1):
        try:
            download_serpset(serpset_id=serpset_id, file_name=serpset_file_name,
                             fetch_params=fetch_params, meta_file_name=meta_file_name)
            return True
        except urequests.RequestForbiddenError as exc:
            handle_fetch_error(serpset_id, fetch_params, str(exc))
            return False
        except urequests.RequestPageNotFoundError as exc:
            handle_fetch_error(serpset_id, fetch_params, str(exc))
            return False
        except urequests.TEMP_ERROR_TYPES as exc:
            logging.warning("Serpset %s fetch error: %s", serpset_id, str(exc))
            logging.info("Waiting %s sec before next download retry", current_retry_timeout)
            time.sleep(current_retry_timeout)
            current_retry_timeout = min(current_retry_timeout * 1.1, max_retry_timeout)

    handle_fetch_error(serpset_id, fetch_params, "{} attempts failed".format(attempts))
    return False
