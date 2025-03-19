import re
import requests

"""
    Client lib for the official kikimr HtmlApi™
"""

NODE_PATTERN = re.compile(r"NodeID: (\d+)")
GEN_PATTERN = re.compile(r"Tablet generation: (\d+)")
TABLET_PATTERN = re.compile(r"TabletID=(\d+)")
CELL_PATTERN = re.compile(r"<td>(.*?)</td>")
BLOB_PATTERN = re.compile(r"\[\d+:\d+:\d+:\d+:\d+:\d+:\d+\]")


def grep(text, pattern):
    m = pattern.search(text)
    if m is None:
        return None
    return m.group(1)


def fetch(url):
    r = requests.get(url)
    r.raise_for_status()
    return r.content


def find_tablet(host, port, tablet):
    url = "http://{}:{}/tablets?TabletID={}".format(host, port, tablet)
    text = fetch(url)

    node = grep(text, NODE_PATTERN)
    if node is None:
        return (0, 0)

    gen = grep(text, GEN_PATTERN)
    if gen is None:
        return (0, 0)
    return (int(node), int(gen))


def fetch_groups(host, port, hive, tablet):
    url = "http://{}:{}/tablets/app?TabletID={}&page=Groups&tablet_id={}".format(
        host,
        port,
        hive,
        tablet)
    text = fetch(url)

    cells = []
    offset = 0
    while offset < len(text):
        m = CELL_PATTERN.search(text, offset)
        if m is None:
            break
        cells.append(m.group(1))
        offset = m.end()

    groups = []
    offset = 0
    while offset < len(cells):
        s = grep(cells[offset], TABLET_PATTERN)
        assert(int(s) == tablet)
        channel = int(cells[offset + 1])
        group = int(cells[offset + 2])
        groups.append((channel, group))
        offset = offset + 6
    return groups


def fetch_blobs(host, port, tablet, channel, group, gen):
    url = "http://{}:{}/blob_range?groupId={}&tabletId={}&from=[{}:0:0:{}:0:0:0]&to=[{}:{}:0:{}:0:0:0]".format(
        host,
        port,
        group,
        tablet,
        tablet,
        channel,
        tablet,
        gen,
        channel)
    text = fetch(url)

    blobs = []
    offset = 0
    while offset < len(text):
        m = BLOB_PATTERN.search(text, offset)
        if m is None:
            break
        blobs.append(m.group(0))
        offset = m.end()
    return blobs


def is_blob_missing(host, port, blob, group_id):
    url = "http://{}:{}/get_blob?blob={}&groupId={}&debugInfo=0".format(
        host,
        port,
        blob,
        group_id
    )

    r = requests.get(url)
    r.raise_for_status()
    return r.status_code == 204 or "NODATA" in r.text


def add_garbage(host, port, tablet, blobs):
    url = "http://{}:{}/tablets/app?TabletID={}&action=addGarbage&blobs={}".format(
        host,
        port,
        tablet,
        blobs)
    fetch(url)

    # TODO: check for error
