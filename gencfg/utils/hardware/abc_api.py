# coding: utf8
import collections

import os
import json
import pickle
import requests
import sys

import resources_counter

from collections import defaultdict

import urllib3
urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)

# use https://oauth.yandex-team.ru/authorize?response_type=token&client_id={token} with id from application on https://oauth.yandex-team.ru

def get_abc_tokent():
    token_path = "~/.abc/token"
    abs_token_path = os.path.expanduser(token_path)
    if not os.path.exists(abs_token_path):
        sys.stderr.write("ABC OAuth key is absent: %s\n" % token_path)
        sys.stderr.write("Read more at https://github.yandex-team.ru/devtools/startrek-python-client#configuring\n")
        exit(1)

    return file(abs_token_path).read().strip()


class AbcApi(object):
    def __init__(self, token=None):
        if not token:
            token = get_abc_tokent()

        self.api_url = 'https://abc-back.yandex-team.ru/api/v4'
        self.token = token

        self.service_tree = {}

        self.id_to_slug = {}
        self.slug_to_id = {}

        self.name_to_slug = defaultdict(set)

    def load_abc_service_tree(self):
        service_tree = {}
        id_to_slug = {}
        slug_to_id = {}

        next_url = None
        while True:
            response_json = self._get('/services/?page_size=1000&fields=id,slug,name,parent,tags', url=next_url)
            for service in response_json['results']:
                service_id = service['id']
                service_slug = service['slug']
                id_to_slug[service_id] = service_slug
                slug_to_id[service_slug.lower()] = service_id

                service_parent_id = None
                service_parent_slug = None
                if service['parent'] is not None:
                    service_parent_id = service['parent']['id']
                    service_parent_slug = service['parent']['slug']
                    id_to_slug[service_parent_id] = service_parent_slug
                    slug_to_id[service_parent_slug.lower()] = service_parent_id

                names = service["name"]
                for language, name in names.iteritems():
                    self.name_to_slug[name].add(service_slug)

                for language in ["ru", "en"]:
                    best_name = names.get(language)

                    if best_name:
                        break
                if not best_name:
                    best_name = service_slug

                tags = [el['slug'] for el in service['tags']]

                service_tree[service_id] = {
                    'id': service_id,
                    'slug': service_slug,
                    'parent_id': service_parent_id,
                    'parent_slug': service_parent_slug,
                    'children': [],
                    'parents': [],
                    'name': best_name,
                    'tags': tags,
                }

            if response_json['next'] is None:
                break
            next_url = response_json['next']

        for id, service in service_tree.iteritems():
            parent_id = service['parent_id']
            if parent_id is None or parent_id not in service_tree:
                continue
            service_tree[parent_id]['children'].append(id)

            while parent_id is not None:
                service_tree[id]['parents'].append(parent_id)
                if parent_id not in service_tree:
                    break
                parent_id = service_tree[parent_id]['parent_id']

        self.service_tree = service_tree
        self.id_to_slug = id_to_slug
        self.slug_to_id = slug_to_id
        return service_tree

    def has_service_info(self, service_slug=None, service_id=None):
        return bool(self.get_service_info(service_slug=service_slug, service_id=service_id))

    def get_service_info(self, service_slug=None, service_id=None):
        assert service_slug is not None or service_id is not None

        if service_id is None:
            service_id = self.get_service_id(service_slug)

        return self.service_tree.get(service_id, {})

    def get_service_id(self, service_slug):
        return self.slug_to_id.get(service_slug.lower())

    def get_service_slug(self, service_id):
        return self.id_to_slug.get(service_id)

    def get_service_slug_by_name(self, service_name):
        if isinstance(service_name, str):
            service_name = service_name.decode('utf-8')

        res = self.name_to_slug.get(service_name)
        if not res:
            return None

        if len(res) > 1:
            sys.stderr.write(
                (u"Multiple slug with name '%s': %s\n" % (service_name, ",".join(sorted(res)))).encode("utf-8"))
            return None

        return list(res)[0]

    def get_service_name(self, service_slug):
        return self.get_service_info(service_slug).get("name")

    def get_service_childern(self, service_slug=None, service_id=None):
        children = self.get_service_childern_ids(service_slug, service_id)
        return [self.get_service_slug(x) for x in children]

    def get_service_childern_ids(self, service_slug=None, service_id=None):
        return self.get_service_info(service_slug, service_id).get('children', [])

    def get_service_parents(self, service_slug=None, service_id=None):
        parents = self.get_service_parents_ids(service_slug, service_id)
        return [self.get_service_slug(x) for x in parents]

    def get_service_parents_ids(self, service_slug=None, service_id=None):
        return self.get_service_info(service_slug, service_id).get('parents', [])

    def get_service_all_children(self, service_slug=None, service_id=None):
        all_childern = self.get_service_all_children_ids(service_slug, service_id)
        return [self.get_service_slug(x) for x in all_childern]

    def get_service_all_children_ids(self, service_slug=None, service_id=None):
        children = self.get_service_childern_ids(service_slug, service_id)

        all_childern = set()
        for child_service_id in children:
            all_childern.add(child_service_id)
            all_childern |= set(self.get_service_childern_ids(service_id=child_service_id))
        return sorted(all_childern)

    def _get(self, api_path=None, query=None, url=None):
        query = query or {}
        request_url = url or '{}{}'.format(self.api_url, api_path)

        for key, value in query.items():
            sep = '&' if '?' in request_url else '?'
            request_url = '{}{}{}={}'.format(request_url, sep, key, value)

        headers = {
            'Authorization': 'OAuth {}'.format(self.token),
            'Accept': 'application/json',
        }

        response = requests.get(request_url, headers=headers, verify=False)
        if response.status_code != 200:
            raise RuntimeError('Response HTTP code {} != 200'.format(response.status_code))

        return response.json()

    def get_service_vs_id(self, service_slug=None, service_id=None):
        parent_id = self.get_service_parents_ids(service_slug=service_slug, service_id=service_id)

        for parent in parent_id:
            if 'vs' in self.service_tree[parent]['tags']:
                return parent

        return None

    def get_service_vs(self, service_slug=None, service_id=None):
        return self.get_service_slug(
            self.get_service_vs_id(service_slug=service_slug, service_id=service_id)
        )

    def get_service_vs_or_top(self, service_slug=None, service_id=None):
        vs = self.get_service_vs(service_slug=service_slug, service_id=service_id)

        if vs:
            return vs

        parents = self.get_service_parents(service_slug=service_slug, service_id=service_id)

        return parents[-1] if parents else service_slug


def load_abc_api(reload=False, no_dump=False):
    res_path = os.path.join(resources_counter.create_resources_dumps_dir(), "abc_api.pickle")
    if not no_dump and not reload and os.path.exists(res_path):
        with file(res_path) as f:
            return pickle.load(f)

    api = AbcApi()
    api.load_abc_service_tree()

    if not no_dump:
        with file(res_path, "wb") as f:
            pickle.dump(api, f)

    return api


def test():
    abcapi = AbcApi(get_abc_tokent())
    d = abcapi.load_abc_service_tree()

    """
    taxiquotaypdev:3042
    edastorageqyp:3073
    vopstaxi:1237
    """
    print(json.dumps(d, indent=4))

    print(sorted(abcapi.get_service_childern_ids(service_id=1975)))
    print(sorted(abcapi.get_service_all_children_ids(service_id=1975)))
