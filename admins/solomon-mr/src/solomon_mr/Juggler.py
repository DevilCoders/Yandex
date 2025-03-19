# This file exists only because juggler-sdk is not in arcadia
from typing import List

import logging
import requests
from attr import attrs, asdict


@attrs(auto_attribs=True)
class JugglerEvent:
    host: str
    service: str
    description: str
    status: str  # OK, WARN, CRIT


class Juggler:
    def __init__(self, token: str):
        self._log = logging.getLogger(self.__class__.__name__)

        self._session = requests.Session()
        self._session.headers = {'Authorization': 'OAuth {}'.format(token)}

    def post_events(self, events: List[JugglerEvent]):
        data = {
            'source': 'solomon-mr',
            'events': []
        }
        for event in events:
            data['events'].append(asdict(event))

        try:
            self._session.post('http://juggler-push.search.yandex.net/events', data=data, timeout=(3, 10))
        except requests.exceptions.RequestException as e:
            self._log.error('Got an error posting to juggler: %s', e)
