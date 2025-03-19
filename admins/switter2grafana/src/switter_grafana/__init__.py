import requests
import os
import yaconfig
import logging
import time
import datetime
import pymongo
import kazoo.client
from transliterate import translit


class SwitterEvent:
    def __init__(self, _id, description, time_string):
        self.id = _id
        self.description = description
        self.timestamp = datetime.datetime.strptime(time_string, "%Y/%m/%d %H:%M:%S")

    def __repr__(self):
        return '<{}>'.format(self.id)


class SwitterManager:
    def __init__(self):
        pkg_dir, _ = os.path.split(__file__)
        self.pem_path = os.path.join(pkg_dir, "yandex.pem")
        self.config = yaconfig.load_config(os.path.join(pkg_dir, 'main.yaml'))

        self.log = logging.getLogger('SwitterManager')
        self.switter_token = self.config.secrets.switter_token
        self.mongo = pymongo.MongoClient(self.config.secrets.mongo_url)
        self.zk = kazoo.client.KazooClient(hosts=self.config.secrets.zk_url)

    def get_switter_events(self):
        """ Fetch events (messages and comments) from switter """
        self.log.debug('Fetching new switter elements')
        result = requests.get('https://switter.yandex-team.ru/switter/api/v1/messages',
                              headers={'Authorization': 'OAuth ' + self.switter_token},
                              verify=self.pem_path,
                              timeout=10)
        if result.status_code != 200:
            raise RuntimeError(result.text)
        messages = result.json()['messages']

        # convert the format
        result = []
        for message in messages:
            result.append(SwitterEvent(message['mid'], message['description'], message['date']))
            for comment in message['comments']:
                result.append(SwitterEvent(comment['_id'], comment['description'], comment['date']))

        return list(sorted(result, key=lambda x: x.timestamp, reverse=True))

    def save_events(self, events):
        """ Saves events to the database """
        db = self.mongo['switter']
        coll = db['events']

        # events are sorted so insert until we find one already present event
        for event in events:
            mongo_event = coll.find_one({'_id': event.id})
            if mongo_event:
                break

            self.log.info('Adding new event {}'.format(event.id))
            coll.insert({'_id': event.id,
                         'description': event.description,
                         'timestamp': event.timestamp,
                         'broadcasted': False})

    def broadcast_events(self):
        db = self.mongo['switter']
        coll = db['events']

        events = coll.find({'broadcasted': False}).sort('timestamp')
        for event in events:
            self.log.info('Broadcasting event {}'.format(event['_id']))
            res = requests.post('https://gr-mg.yandex-team.ru/events/',
                                json={
                                    'tags': 'nyanbot.switter',
                                    'what': 'Switter event',
                                    'when': event['timestamp'].timestamp(),
                                    'data': translit(event['description'], 'ru', reversed=True),
                                })
            if res.status_code == 200:
                coll.update({'_id': event['_id']}, {'$set': {'broadcasted': True}})

    def run_tick(self):
        """ Runs one tick """
        self.save_events(self.get_switter_events())
        self.broadcast_events()

    def run(self):
        """ The main daemon loop """
        self.log.info('Starting the manager')
        self.zk.start()
        lock = self.zk.Lock("/switter2grafana")
        with lock:
            self.log.info("Acquired the ZK lock")
            try:
                while True:
                    self.run_tick()
                    time.sleep(60)
            except KeyboardInterrupt:
                pass
        self.zk.stop()
        self.log.info('Manager stopped')
