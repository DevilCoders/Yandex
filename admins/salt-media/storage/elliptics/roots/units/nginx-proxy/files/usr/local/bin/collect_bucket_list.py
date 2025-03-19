import requests
import json
import logging
import sys
from requests.packages.urllib3.exceptions import (
    InsecureRequestWarning,
    InsecurePlatformWarning,
    SNIMissingWarning,
)
import os
from boto.s3.connection import S3Connection


requests.packages.urllib3.disable_warnings(InsecureRequestWarning)
requests.packages.urllib3.disable_warnings(InsecurePlatformWarning)
requests.packages.urllib3.disable_warnings(SNIMissingWarning)

LOGFILE="/var/log/collect_buckets_script.log"
logging.basicConfig(filename=LOGFILE, encoding='utf-8', level=logging.ERROR, format='%(asctime)s: %(message)s', datefmt='%Y-%m-%d %H:%M:%S')
locallogger = logging.getLogger('main')
locallogger.setLevel(logging.DEBUG)

environment = None
with open("/etc/yandex/environment.type") as f:
    environment = f.read().strip()

endpoint = {"testing": "mdst",
            "prestable": "mds",
            "production": "mds"}


def upload_to_s3(data):
    os.environ['S3_USE_SIGV4'] = 'True'
    with open("/etc/mds-hide/config.json") as f:
        config = json.load(f)

    conn = S3Connection(
        aws_access_key_id=config["s3"]["access_key"],
        aws_secret_access_key=config["s3"]["secret_key"],
        host='s3.{}.yandex.net'.format(endpoint[environment])
    )
    conn.auth_region_name = 'us-east-1'
    bucket = conn.get_bucket('mds-service')

    bucket.new_key('buckets_names').set_contents_from_string(data)


def get_storage_version():
    response = requests.get(
        "https://s3.{}.yandex.net/mds-service/buckets_names".format(
            endpoint[environment])
    )
    return response.text


def get_data():
    data = []
    try:
        url = "stats/buckets"
        while True:
            uri = "https://s3-idm.{}.yandex.net/{}".format(endpoint[environment], url)
            response = requests.get(
                uri,
                verify=False)
            locallogger.debug("Get data with uri: {0}, data len is: {1}".format(uri, len(response.text)))
            metrics = json.loads(response.text)
            for info in metrics["results"]:
                data.append(info)
            if metrics["next"] is None:
                break
            url = "stats/buckets?cursor={}".format(metrics["next"])
    except Exception as e:
        locallogger.error("Catch with uri: {0} err: {1}".format(uri, e))
        return None
    return data


def make_buckets_list(data):
    buckets_names = []
    try:
        if data:
            for bucket_stats in data:
                try:
                    buckets_names.append(bucket_stats["name"])
                except Exception as e:
                    locallogger.error("In make_buckets_list() cant fet bucket_stats[name]: err: {0}, bucket_stats: {1}, buckets_names type and size: {2} {3}".format(e, bucket_stats, type(buckets_names), len(buckets_names)))
                    return None
    except Exception as e:
        locallogger.error("Cant make_buckets_list(): err: {0} {1}".format(e,data))
        return None

    return buckets_names


def write_to_file(data):
    text_to_write = "\n".join(data)
    locallogger.debug("text_to_write data size is: {0}".format(len(data)))

    if text_to_write:
        upload_to_s3(text_to_write)
    else:
        text_to_write = get_storage_version()

    with open("/var/cache/yasm/buckets_names", "w") as f:
        f.write(text_to_write)

    with open("/var/cache/yasm/buckets_names_num", "w") as f:
        size = len(text_to_write.split())
        locallogger.debug("/var/cache/yasm/buckets_names_num data is: {0}".format(size))
        f.write(str(size))


def data_is_good(data):
    threshold = 5  # percentage
    buckets_num_before = 0
    try:
        with open("/var/cache/yasm/buckets_names_num") as f:
            raw_num = str(f.read())
            locallogger.debug("Last /var/cache/yasm/buckets_names_num is: {0}".format(raw_num))
            buckets_num_before = 0
            if len(raw_num) > 0:
                buckets_num_before = int(raw_num)
    except Exception as e:
        locallogger.error("Cant data_is_good(): err: {0}".format(e))
        return False

    buckets_diff = abs(buckets_num_before - len(data) / (len(data) / 100))
    locallogger.debug("len(data) inside data_is_good() is: {0}".format(len(data)))

    if buckets_num_before == 0:
        locallogger.info("data_is_good(): bbf=0")
        return True

    if (buckets_num_before > len(data)) and (buckets_diff > threshold):
        locallogger.info("data_is_good(): diff bigger tr")
        return False

    return True


def main():
    locallogger.info("Started.")
    buckets_data = get_data()
    if not buckets_data:
        locallogger.error("main(): buckets_data is like None: {0}".format(buckets_data))
        return 0
    locallogger.info("buckets_data with size: {0}".format(len(buckets_data)))
    buckets_list = make_buckets_list(buckets_data)
    if not buckets_list:
        locallogger.error("main(): buckets_list is like None: {0}".format(buckets_list))
        return 0
    locallogger.info("buckets_list with size: {0}".format(len(buckets_list)))
    if data_is_good(buckets_data):
        write_to_file(buckets_list)
    locallogger.info("Ended.")

if __name__ == "__main__":
    sys.exit(main())
