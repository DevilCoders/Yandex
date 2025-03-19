# -*- coding=utf-8 -*-
import itertools
import re
from pprint import pprint

import requests
import asyncio
import aiohttp
import logging


logger = logging.getLogger(__name__)


class NetboxAuth(requests.auth.AuthBase):
    def __init__(self, token):
        self.token = token

    def __call__(self, r):
        if self.token:
            r.headers["Authorization"] = "Token {}".format(self.token)
        return r


class Netbox:
    def __init__(self, url, token=None):

        if re.match("^.*://", url):
            self.url = url.rstrip("/")
        else:
            self.url = "https://{}".format(url.rstrip("/"))

        self.token = token
        self.session = requests.Session()
        self.session.headers.update({'Accept': 'application/json'})
        self.session.headers.update({'Content-Type': 'application/json'})
        self.session.auth = NetboxAuth(self.token)
        print(self.url)

    def get(self, request):
        request = request.lstrip('/')
        url = self.url + '/' + request
        # logging.debug('Netbox::get() url: ' + url)
        logger.debug('Netbox::get() url: ' + url)

        res = self.session.get(url, verify=False)
        if res.status_code != 200:
            logger.error(f'Netbox::get() {res.status_code}  {res.content}')
            exit()

        return res

    def get_devices_list(self, filter=""):
        response = self.get(f'dcim/devices/?{filter}&limit=0').json()
        devices = {}
        for x in response['results']:
            print(x['name'])
            devices[x['name']] = {'rack': x['rack']['name'], 'site': x['site']['name']}

        return devices
    
    def get_devices(self, filter=""):
        response = self.get(f'dcim/devices/?{filter}&limit=0').json()
        devices = response['results']
        return devices

    def get_addresses_by_device_names(self, devices: dict = None):
        urls = []
        if devices is None:
            return urls

        for device in devices.keys():
            urls.append(f"{self.url}/ipam/ip-addresses/?device={device}&limit=0")

        return urls

    def get_devices_location(self, devices: dict):
        sites_list = list(set([x['site'] for x in devices.values()]))
        buildings = self.get_sites_parents(sites_list)

        buildings_list = list(set(x for x in buildings.values()))
        dc = self.get_regions_parents(buildings_list)

        for site in buildings.keys():
            for device in devices.keys():
                if devices[device]['site'] == site:
                    devices[device]['building'] = buildings[site]
                    devices[device]['dc'] = dc[buildings[site]]

        return devices

    def get_sites_parents(self, sites) -> dict:
        parents = {}
        for site in sites:
            parents[site] = {'url': f"{self.url}/dcim/sites/?name={site}"}
        loop = asyncio.get_event_loop()
        r = loop.run_until_complete(self.get_bunch(list(set(x['url'] for x in parents.values()))))

        try:
            for x in itertools.chain.from_iterable(r):
                parents[x['name']] = x['region']['name']
        except TypeError as e:
            print(e)
            print(e.args)
            print(r)
            exit()

        return parents

    def get_regions_parents(self, regions) -> dict:
        parents = {}
        for region in regions:
            parents[region] = {'url': f"{self.url}/dcim/regions/?name={region}"}
        loop = asyncio.get_event_loop()
        r = loop.run_until_complete(self.get_bunch(list(set(x['url'] for x in parents.values()))))
        try:
            for x in itertools.chain.from_iterable(r):
                parents[x['name']] = x['parent']['name']
        except TypeError as e:
            print(e)
            print(e.args)
            print(r)
            exit()

        return parents

    async def get_async(self, sessions, url):
        try:
            async with sessions.get(url, timeout=60) as response:
                result = await response.json()
        except:
            pprint(response)
            print(response.content)
            exit()
        return result['results']

    async def get_async_loop(self, session, urls):
        results = await asyncio.gather(*(self.get_async(session, url) for url in urls),
                                       return_exceptions=True)
        return results

    async def get_bunch(self, urls):
        connector = aiohttp.TCPConnector(limit=60, verify_ssl=False)
        async with aiohttp.ClientSession(connector=connector) as session:
            results = await self.get_async_loop(session, urls)
        return results

