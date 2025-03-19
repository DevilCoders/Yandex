#! /usr/bin/python
""" Set juggler downtime for containers """
# XXX Documentation https://juggler-sdk.n.yandex-team.ru/
import os
import re
import time
from argparse import ArgumentParser, HelpFormatter
from collections import OrderedDict

import requests
import yaml
from requests.adapters import HTTPAdapter
from requests.exceptions import RequestException
from requests.packages.urllib3.util import Retry

from juggler_sdk import DowntimeSelector, JugglerApi
from sandbox.projects.common.nanny.client import NannyApiException, NannyClient
from yp.client import YpClient

DURATIONS = OrderedDict(
    (
        ("s", 1),
        ("m", 60),
        ("h", 3600),
        ("d", 3600*24),
        ("w", 3600*24*7),
    )
)


def parse_duration(duration_text):
    if duration_text[-1] not in DURATIONS:
        raise RuntimeError("Only supported duration suffixes %s" % DURATIONS.keys())
    duration = float(duration_text[:-1])
    factor = DURATIONS[duration_text[-1]]
    deadline = time.time() + duration * factor
    return deadline


class TokenManager(object):

    systems = ("qloud", "yp", "nanny", "abc")
    tokens_file = "~/.runtime.yaml"
    common_token_env = "YDOWNTIMER_OAUTH"
    common_token_id = "2ca3c112656c4bfb9c2aef63bac9f75d"
    common_token_url = "https://oauth.yandex-team.ru/authorize?" \
                       "response_type=token&client_id=%s" % common_token_id

    def __init__(self):
        self._tokens = self._load_tokens()
        for system in self.systems:
            setattr(self, system, self._find_token(system))

    def fail_no_token(self):
        raise RuntimeError("No %s variable with token, you can get it here: %s" %
                           (self.common_token_env, self.common_token_url))

    def _load_tokens(self):
        tokens = {
            "common": os.getenv(self.common_token_env),
            "per_system": {}
        }
        try:
            tokens["per_system"] = {x.replace("_token", ""): y for x, y in
                                    yaml.safe_load(open(os.path.expanduser(self.tokens_file))).items()}
        except IOError:
            if not tokens["common"]:
                self.fail_no_token()
            pass
        if not tokens.get("common"):
            banner = "Warning! %s is legacy\nPlease, migrate to %s env variable\n" \
                     "You can get new token for all systems here: %s" % (self.tokens_file,
                                                                         self.common_token_env,
                                                                         self.common_token_url)
            print(banner)
        return tokens

    def _find_token(self, slug):
        token = self._tokens["common"]
        if not token:
            token = self._tokens["per_system"].get(slug)
        return token

    @property
    def juggler(self):
        token = self._find_token("juggler")
        if not token:
            self.fail_no_token()
        return token


class HttpClient(object):

    def __init__(self, address=None, token=None):
        self.address = address
        self.token = token

    def make_request(self, endpoint, method="get", timeout=10, **kwargs):
        headers = {}
        if self.token:
            headers = {"Authorization": "OAuth {}".format(self.token)}
        headers.update(kwargs.get("headers", {}))
        url = self.address + endpoint
        retries = Retry(total=3, backoff_factor=0.1,
                        status_forcelist=[503, 429])
        session = requests.Session()

        for proto in ["http", "https"]:
            session.mount("{}://".format(proto),
                          HTTPAdapter(max_retries=retries))

        data = getattr(session, method)(url, timeout=timeout, headers=headers,
                                        **kwargs)
        try:
            data.raise_for_status()
        except RequestException as error:
            raise RuntimeError("HTTP Error while communicating "
                               "with %s: %s" % (self.address, error))
        return data


class QloudClient(HttpClient):

    def __init__(self, token, address="https://platform.yandex-team.ru/api"):
        super(QloudClient, self).__init__(address=address, token=token)

    @staticmethod
    def _hosts_in_dc(component, dc):
        return [
            instance["host"].encode("utf-8") for instance in component["runningInstances"]
            if instance.get("line") and instance["line"][:3] == dc.upper()
        ]

    def hosts_in_dc(self, application_id, dc):
        if len(dc) != 3:
            raise RuntimeError("DC slug must be 3 symbol length")
        if len(application_id.split(".")) != 3:
            raise RuntimeError("ApplicationId format: "
                               "<project>.<application>.<environment>")

        endpoint = "//v1/environment/stable/" + application_id
        data = self.make_request(endpoint)
        result = []
        for _, component in data.json()["components"].items():
            result += self._hosts_in_dc(component, dc)
        return result

    def environments(self, project):
        endpoint = "/v1/project/" + project
        data = self.make_request(endpoint)
        result = []
        for application in data.json()["applications"]:
            if application["environmentStatuses"].get("REMOVED", 0) == 1:
                continue
            for environment in application["environments"]:
                result.append(environment["objectId"])
        return result


class AbcClient(HttpClient):

    def __init__(self, token, address="https://abc-back.yandex-team.ru/api/v4"):
        super(AbcClient, self).__init__(address=address, token=token)

    def id_from_slug(self, slug):
        endpoint = "/services/"
        params = {"slug": slug}
        try:
            return self.make_request(endpoint, params=params).json()["results"][0]["id"]
        except IndexError:
            raise RuntimeError("No service with slug %s" % slug)


class Downtimer(object):

    dump_file = "dump.yaml"

    def __init__(self):
        tokens = TokenManager()
        self.options = self._parse_options()
        self.config = self._read_config(self.options.config)
        self.juggler = JugglerApi("http://juggler-api.search.yandex.net", oauth_token=tokens.juggler)

        self.yp = YpClient(self.options.datacenter, config={"token": tokens.yp}) if tokens.yp else None
        self.nanny = NannyClient("https://nanny.yandex-team.ru", tokens.nanny) if tokens.nanny else None
        self.qloud = QloudClient(tokens.qloud) if tokens.qloud else None
        self.abc = AbcClient(tokens.abc) if tokens.abc else None

        if self.options.action == "dump":
            self.dump(self.options.datacenter)
        elif self.options.action == "set":
            self.set_dt()
        elif self.options.action == "remove":
            self.remove_dt()

    @staticmethod
    def _parse_options():
        ap = ArgumentParser(formatter_class=lambda prog: HelpFormatter(prog, width=120))
        ap.add_argument("config")
        ap.add_argument("action", choices=["dump", "set", "remove"])
        ap.add_argument("datacenter", help="DC name i.e myt, sas, vla ...")
        ap.add_argument("time", nargs="?", type=parse_duration, default="3h",
                        help="Time duration in form N<suffix> where suffix in {}".format(DURATIONS.keys()))
        args = ap.parse_args()
        if args.action == "set":
            assert args.time is not None, "Duration required one of '{}'".format(DURATIONS.keys())
            print("Downtime deadline {}".format(time.strftime("%FT%T", time.localtime(args.time))))
        return args

    @staticmethod
    def _read_config(file_name):
        return yaml.safe_load(open(file_name))

    def dump(self, dc_filter):
        """ Dump the list to the dump json """
        result = []
        system_mapping = {
            "conductor": self._conductor_containers,
            "nanny": self._rtc_containers,
            "qloud": self._qloud_containers,
            "yp": self._yp_containers
        }
        for system, projects in self.config.items():
            if system == "qloud-envs":
                continue
            if system != "conductor" and not getattr(self, system):
                raise RuntimeError("No client/token for %s" % system)
            for project in projects:
                try:
                    func = system_mapping[system]
                except KeyError:
                    raise RuntimeError("Unknown system: %s, supported: %s" %
                                       (system, list(system_mapping.keys())))
                result.extend(func(project, dc_filter))

        with open(self.dump_file, "w") as f:
            f.write(yaml.safe_dump(result))

    @staticmethod
    def _match_dc_by_hostname(hostname, dc_filter):
        """
        Function get string hostname and match dc.
        :param hostname: str e.g.: mediabilling-test-connector-sas-6.sas.yp-c.yandex.net
        :param dc_filter: str one of: sas, vla, man, iva, myt
        :return: bool
        """
        regexp = re.compile(r"\.([^.]+)\.yp-c.yandex.net$")
        dc = None
        try:
            dc = re.search(regexp, hostname).group(1)
        except AttributeError:
            pass
        return dc == dc_filter

    def _rtc_containers(self, config, dc_filter):
        result = []
        bans = [re.compile(x) for x in config.get("exclude", [])]

        full_list = self.nanny.list_services_by_category(config["include"])["result"]
        for service in full_list:
            info = service["info_attrs"]["content"]
            category = info["category"]
            banned = False
            for ban in bans:
                if ban.search(category) or ban.search(service["_id"]):
                    banned = True
                    break

            for label in info["labels"]:
                if label["key"] == "dc":
                    if label["value"].lower() != dc_filter:
                        banned = True

            if not banned:
                result.append(service["_id"])
        hosts = []
        hostname_matcher = config.get("match_dc_by_hostname", False)
        for service in result:
            try:
                resp = self.nanny.get_service_current_instances(service)
                insts = []
                for x in resp["result"]:
                    inst = x["container_hostname"].encode("utf-8")
                    if hostname_matcher and not self._match_dc_by_hostname(inst, dc_filter):
                        continue
                    insts.append(inst)
                if not insts:
                    continue
                hosts.extend(insts)
                print("Resolved nanny {} to {}".format(service, insts))
            except NannyApiException as exc:
                print("Ignore: {}".format(exc))

        return hosts

    @staticmethod
    def _conductor_containers(project, dc_filter):
        resp = requests.get("https://c.yandex-team.ru/api/projects2hosts/" + project +
                            "?fields=fqdn,root_datacenter_name&format=json")
        resp.raise_for_status()
        hosts = [x["fqdn"].encode("utf-8") for x in resp.json() if x["root_datacenter_name"] == dc_filter]
        print("Resolved conductor {} to {}".format(project, hosts))
        return hosts

    def _qloud_containers(self, project, dc_filter):
        hosts = []
        envs = self.qloud.environments(project)
        for env in envs:
            should_resolve = True
            if self.config.get("qloud-envs"):
                should_resolve = any(conf_env in env for conf_env in self.config["qloud-envs"])
            if should_resolve:
                _hosts = self.qloud.hosts_in_dc(env, dc_filter)
                hosts += _hosts
                print("Resolved qloud env {} to {}".format(env, _hosts))
        return hosts

    def _resolve_abc_slug(self, slug):
        if not isinstance(slug, str):
            return slug
        if not self.abc:
            raise RuntimeError("No suitable token for abc")
        return self.abc.id_from_slug(slug)

    def _yp_containers(self, project, dc_filter):
        containers = []
        project = self._resolve_abc_slug(project)
        pod_sets_filter = "[/spec/account_id] IN ('abc:service:%s') AND " \
                          "([/labels/deploy_engine] = 'MCRSC' OR [/labels/deploy_engine] = 'RSC')" % project
        pod_sets = self.yp.select_objects("pod_set", filter=pod_sets_filter, selectors=["/meta/id"])
        for pod_set in pod_sets:
            pod_set = pod_set[0]
            pod_set_filter = "[/meta/pod_set_id]='%s'" % pod_set
            fqdns = self.yp.select_objects("pod", filter=pod_set_filter, selectors=["/status/dns/persistent_fqdn"])
            fqdns = [x.decode("utf-8") for y in fqdns for x in y]
            print("Resolved yp podset {} to {}".format(pod_set, fqdns))
            containers.extend(fqdns)
        return containers

    def set_dt(self):
        items = yaml.safe_load(open(self.dump_file))

        while items:
            pool = items[:50]
            items = items[50:]
            print("Setting downtimes for {}".format(pool))
            self.juggler.set_downtimes([DowntimeSelector(x) for x in pool], end_time=self.options.time)

    def remove_dt(self):
        items = yaml.safe_load(open(self.dump_file))

        while items:
            pool = items[:50]
            items = items[50:]
            print("Removing downtimes for {}".format(pool))
            lst = self.juggler.get_downtimes([DowntimeSelector(x) for x in pool])
            ids = [x.downtime_id for x in lst.items]
            self.juggler.remove_downtimes(ids)


def main():
    Downtimer()


if __name__ == "__main__":
    main()
