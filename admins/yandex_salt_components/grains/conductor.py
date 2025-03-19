import logging
import socket
import requests

log = logging.getLogger(__name__)

API_BASE = "http://c.yandex-team.ru/api-lightcached"


def _get_conductor_data(s: requests.Session, url: str):
    log.debug(f"fetching {url}")
    try:
        response = s.get(url, timeout=5)
        response.raise_for_status()
        data = response.json()
        log.debug("response {0}".format(response))
    except Exception as e:
        log.exception(f"failed fetch info from {url}: {e}")
    else:
        return data


def _conductor_info(s: requests.Session):
    # use __opts__["id"] instead ???
    fqdn = socket.getfqdn()

    hostinfo = _get_conductor_data(s, f"{API_BASE}/hosts/{fqdn}/?format=json")
    if hostinfo is None:
        log.warning("Host is not configured in conductor.")
        return

    info = hostinfo[0]
    tags = _get_conductor_data(s, f"{API_BASE}/get_host_tags/{fqdn}/?format=json")
    groups = _get_conductor_data(s, f"{API_BASE}/hosts2groups/{fqdn}/?format=json")
    project = _get_conductor_data(s, f"{API_BASE}/hosts2projects/{fqdn}/?format=json")

    c = []
    if tags:
        info["tags"] = tags
    if groups:
        info["groups"] = [x["name"] for x in groups]
        for group in groups:
            c.append(format(group["name"]))
            c.append("{0}@{1}".format(group["name"], info["root_datacenter"]))

    if project:
        project_name = project[0]["name"]
        info["project"] = project_name
        project_info = _get_conductor_data(
            s, f"{API_BASE}/projects/{project_name}/?format=json"
        )
        if project_info:
            info["project_info"] = project_info[0]

    result = {"conductor": info, "c": c}
    return result


def conductor_info():
    with requests.Session() as s:
        data = _conductor_info(s)
    return data
