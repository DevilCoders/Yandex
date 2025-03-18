import json
import re

import library.python.resource as res


class ServiceIdentifier:
    def __init__(self, config=None):
        if config is None:
            config = json.loads(res.find("service_identifier.json").decode("utf-8"))

        self.services = []
        for service_info in config:
            service = service_info["host"]
            regex = service_info["path"] + service_info.get("query", "")
            service_regex = re.compile(regex)
            self.services.append((service, service_regex))

    def get_service(self, url):
        for prefix in ("www.", "m.", "pda.", "wap."):
            if url.startswith(prefix):
                url = url[len(prefix):]

        for service, identifier in self.services:
            if identifier.fullmatch(url):
                return service

        return "other"
