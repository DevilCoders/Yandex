import json

import requests
import yt.wrapper as yt

from value_streams import value_streams


def get_vs_parent(path):
    parts = path.split("/")
    for part in reversed(parts):
        if part in value_streams:
            return value_streams[part]
    return None


def write_table(data, dst_table):
    table_path = yt.TablePath(dst_table)
    yt.write_table(
        table_path,
        '\n'.join(map(json.dumps, data)).encode("utf-8"),
        raw=True,
        format=yt.JsonFormat(attributes={"encode_utf8": False})
    )


def get_from_abc_and_enrich(abc_url, abc_token):
    results = []
    while True:
        response = requests.get(abc_url,
                                headers={"Authorization": "OAuth {}".format(abc_token)})
        for entry in response.json()["results"]:
            vs = get_vs_parent(entry["path"])
            if vs:
                new_entry = {"abc_name": entry["slug"]}
                new_entry.update(vs)
                results.append(new_entry)
        if response.json()["next"] is None:
            break
        abc_url = response.json()["next"]

    return results


def write_enriched(dst_table, yt_proxy, yt_token, abc_url, abc_token):
    yt.config['token'] = yt_token
    yt.config['proxy']['url'] = yt_proxy
    data = get_from_abc_and_enrich(abc_url=abc_url, abc_token=abc_token)
    write_table(data, dst_table)
