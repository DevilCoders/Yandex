import json
import collections

import library.python.resource


def get_schema(name):
    resource = library.python.resource.find("service_config.json")
    return json.loads(resource)


FLAG_KINDS = {
    "cbb_ip_flag": "",
    "cbb_re_flag": "403 by regex",
    "cbb_re_mark_flag": "",
    "cbb_re_mark_log_only_flag": "",
    "cbb_re_user_mark_flag": "",
    "cbb_captcha_re_flag": "",
    "cbb_farmable_ban_flag": "",
}


COMMON_FLAGS = {
    185: "regex for markup",
    225: "antirobot_force_captcha_ip",
    226: "antirobot_force_captcha_all",
    262: "antirobot_force_captcha_text",
    702: "regex for markup log only",
}


class AntirobotGroups:
    def __init__(self):
        self.config = collections.defaultdict(dict)
        self.all_services = set()

        resource = library.python.resource.find("service_config.json")
        for section in json.loads(resource):
            service = section["service"]

            for kind in FLAG_KINDS:
                flags = section.get(kind)
                if flags:
                    if not isinstance(flags, list):
                        flags = [flags]
                    for flag in flags:
                        self.config[flag][service] = FLAG_KINDS[kind]
                        self.all_services.add(service)

        for flag in COMMON_FLAGS:
            for service in self.all_services:
                self.config[flag][service] = COMMON_FLAGS[flag]

    def get_services(self, group_id):
        return sorted(self.config[group_id].keys())

    def is_known_service(self, candidate):
        return candidate.lower() in self.all_services

    def get_all_services(self):
        return sorted(self.all_services)
