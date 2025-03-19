#!/usr/bin/env python2
from jinja2 import Template
from admins.pylib.common import nanny_client


class GenerateL7Upstream:
    def __init__(self, filename_tmpl, services):
        self.services = services
        self.nanny = nanny_client()
        self.filename = filename_tmpl

    def run(self):
        template = Template(self.read_template(self.filename))
        hosts = dict()
        for dc in self.services.keys():
            instances = self.instances_for_group(self.services[dc])
            hosts[dc] = instances
        slb_config = template.render(hosts=hosts)
        self.write_to_file(self.filename, slb_config)
        print("done")

    def instances_for_group(self, groups):
        instances = list()
        for group in groups:
            print("Getting gencfg config for group {}".format(group))
            instances = instances + [host["container_hostname"]
                                     for host in self.nanny.get_service_current_instances(group)["result"]]
        instances.sort()
        return instances

    @staticmethod
    def read_template(filename):
        template = filename + ".jinja2"
        with open(template, 'r') as f:
            content = f.read()
        return content

    @staticmethod
    def write_to_file(config, template):
        config = config + ".yml"
        with open(config, "wb") as f:
            print("Creating {}".format(config))
            f.write(template.lstrip())
