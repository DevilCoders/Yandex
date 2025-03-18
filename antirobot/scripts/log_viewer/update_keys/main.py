# coding: utf-8

import os
import requests
from yalibrary.yandex import sandbox


def store_keys(url, dst_file_name):
    response = requests.get(url)
    assert response.status_code == 200
    with open(dst_file_name, "w") as out:
        out.write(response.content.decode())


def main():
    sandbox_client = sandbox.SandboxClient(sandbox_url=sandbox.DEFAULT_SANDBOX_URL, token=os.environ["SANDBOX_TOKEN"])
    res = sandbox_client.last_resources(["ANTIROBOT_KEYS"])[0]
    print("Resource details:")
    print(res)
    if not res:
        raise Exception("Could not find antirobot keys resource")

    store_keys(res["http"]["proxy"], os.environ.get("KEYS_FILE", "keys"))


if __name__ == "__main__":
    main()
