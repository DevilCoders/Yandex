#!/usr/bin/env python3

import yaml
from copy import deepcopy


INPUT_YES = 'yes'
ERR_CODE = 1
CONTROLPLANE_REPO = 'https://bb.yandex-team.ru/projects/MDB/repos/controlplane/'
INPUT_PROMPT = '''
Type "{yes}" if you have already followed these steps:
1. Created a PR in GERRIT for files located near helm/stands/resources_for_all_stands/default_version/default_versions_config.yaml in {repo}.
2. PR has been approved and deployed by ArgoCD installation to preprod (and other environments under k8s management). Ask CORE team for URL of ARGOCD.
3. If everything is fine, then you continue using this script as always for all other enviroments:
   1. run this script there
   2. commit changes
'''.format(
    yes=INPUT_YES,
    repo=CONTROLPLANE_REPO,
).strip()

env_to_number = {
    'dev': 0,
    'qa': 1,
    'prod': 2,
    'compute-prod': 3,
}


class MyDumper(yaml.SafeDumper):
    def write_line_break(self, data=None):
        super().write_line_break(data)

        if len(self.indents) == 4:
            super().write_line_break()

    def ignore_aliases(self, _):
        return True


class Versions:
    def __init__(self, versions, updates):
        self.versions = versions
        self.updates = dict()
        for update in updates:
            component = update['component']
            _type = update['type']
            envs = update.get('env', env_to_number.keys())
            for env in envs:
                key = (component, _type, env)
                self.updates[key] = dict()
                for edge in update['edges']:
                    _from = edge['from']
                    _to = edge['to']
                    self.updates[key][_from] = _to

    def env(self):
        result = []
        for version in self.versions:
            if 'env' in version:
                if type(version['env']) is list:
                    for env in version['env']:
                        new_version = deepcopy(version)
                        new_version['env'] = env
                        result.append(new_version)
                else:
                    result.append(version)
        self.versions = result
        return self

    def is_default(self):
        for version in self.versions:
            if 'is_default' not in version:
                version['is_default'] = False
        return self

    def is_deprecated(self):
        for version in self.versions:
            if 'is_deprecated' not in version:
                version['is_deprecated'] = False
        return self

    def edition(self):
        result = []
        for version in self.versions:
            if 'edition' not in version:
                version['edition'] = 'default'
                result.append(version)
            elif type(version['edition']) is list:
                for edition in version['edition']:
                    new_version = deepcopy(version)
                    new_version['edition'] = edition
                    result.append(new_version)
            else:
                result.append(version)
        self.versions = result
        return self

    def major_version(self):
        result = []
        for version in self.versions:
            if 'major_version' in version:
                if type(version['major_version']) is list:
                    for major_version in version['major_version']:
                        new_version = deepcopy(version)
                        new_version['major_version'] = major_version
                        result.append(new_version)
                else:
                    result.append(version)
        self.versions = result
        return self

    def name(self):
        for version in self.versions:
            if 'name' not in version:
                if version['edition'] != 'default':
                    version['name'] = '{major}-{edition}'.format(major=version['major_version'],
                                                                 edition=version['edition'])
                else:
                    version['name'] = str(version['major_version'])
        return self

    def updatable_to(self):
        for version in self.versions:
            if 'updatable_to' not in version:
                version['updatable_to'] = None
                component = version['component']
                _type = version['type']
                name = version['name']
                env = version['env']
                key = (component, _type, env)
                if key not in self.updates or name not in self.updates[key]:
                    continue
                version['updatable_to'] = self.updates[key][name]
        return self


def update_default_versions(versions):
    yaml.representer.ignore_aliases = lambda *data: True
    with open("metadb_default_versions.sls", "w") as file:
        file.write('# This file is generated from default_versions_config.yaml using generate_default_versions.py\n')
    with open("metadb_default_versions.sls", "a") as file:
        yaml.dump({'data': {'dbaas_metadb': {'default_versions': versions}}}, file, default_flow_style=False,
                  indent=2, Dumper=MyDumper)


if __name__ == '__main__':
    if input(INPUT_PROMPT + "\n --> ") != INPUT_YES:
        print("Please, do.")
        exit(ERR_CODE)

    with open("default_versions_config.yaml", "r") as file:
        config = yaml.safe_load(file)

    versions = Versions(config['versions'], config['updates'])

    result = versions\
        .env()\
        .is_default()\
        .is_deprecated()\
        .edition()\
        .major_version()\
        .name()\
        .updatable_to()

    result.versions.sort(key=lambda version: (version['type'],
                                              version['component'],
                                              version['name'],
                                              env_to_number.get(version['env'], version['env'])))

    update_default_versions(result.versions)
