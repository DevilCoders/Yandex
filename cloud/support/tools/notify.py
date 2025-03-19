#!/usr/bin/env python
import csv
import json
import logging
import os
from datetime import datetime

import click
import requests

logger = logging.getLogger(__name__)
logging.basicConfig(format='|%(asctime)s| %(message)s',
                    level=logging.INFO, datefmt='%H:%M:%S',
                    handlers=[logging.FileHandler("notifylog.log"), logging.StreamHandler()])
BLANK_JSON = {"subject": {"ru": "", "en": ""}, "type": "technical-work", "data": {
    "content": {"ru": "", "en": ""}}}
USER_AGENT = 'https://bb.yandex-team.ru/projects/CLOUD/repos/support-tools/browse/notify.py'
warnlist = {}


# IAM API
class IamAPI:
    def __init__(self):
        self.endpoint = 'https://identity.private-api.cloud.yandex.net:14336/v1'
        self.session = requests.Session()
        self.session.headers.update({
            'User-Agent': USER_AGENT
        })

    def get_email(self, cloud_id):
        response = self.session.get(
            self.endpoint+'/clouds:creators?cloudId={}&showBlocked=True'.format(cloud_id))
        creators = response.json()['cloudCreators']
        if not creators:
            logger.warning(
                'IAM API Response: There is no linked email for: {}'.format(cloud_id))
            return None
        if creators[0]['cloud']['id'] == cloud_id:
            email = (creator.get('userSettings').get('email')
                     for creator in creators)
            return email
        else:
            logger.warning(
                'IAM API Response {} doesnt match requested cloudId {}'.format(creators, cloud_id))


# Notify API
class NotifyAPI:
    def __init__(self):
        os.environ['WEBSOCKET_CLIENT_CA_BUNDLE'] = 'allCAs.pem'
        self.endpoint = 'https://console.cloud.yandex.net/notify/api'
        self.session = requests.Session()
        self.session.headers.update({
            'User-Agent': USER_AGENT
        })

    def mail(self, email, data, subject, message_type):
        post_data = {'receiver': email, 'subject': subject,
                     'type': message_type, 'data': data}
        try:
            response = self.session.post(
                '{}/{}'.format(self.endpoint, 'send'), json=post_data)
        except Exception as e:
            logger.warning("Exception {} raised".format(e))
            logger.info("Trying again")
            try:
                response = self.session.post(
                    '{}/{}'.format(self.endpoint, 'send'), json=post_data)
            except Exception as f:
                logger.warning("Second try failed")
                warnlist[email] = f
        logger.info('Notify API response: {}'.format(response.text))
        try:
            response.raise_for_status()
        except Exception as e:
            logger.info('ERROR {}'.format(e))
            warnlist[email] = e


@click.group()
def cli():
    pass


#  Script config init
@click.command()
def init():
    try:
        cert = requests.get("https://crls.yandex.net/allCAs.pem")
        with open("allCAs.pem", "wb") as ca:
            ca.write(cert.content)
        with open('notifyclouds.csv', 'w') as cloud_list:
            cloud_list.write('cloud_id\n')
        with open('notifyconfig.json', 'w') as config:
            config.write(json.dumps(BLANK_JSON))
        logger.info('Dont forget to fill clouds list and edit config file')
        logger.info(
            'Visit https://github.yandex-team.ru/data-ui/notify for API Docs')
    except Exception:
        logger.warning('Error! Contact @hexyo')


# Supports mail send function
@click.command()
@click.option('--clouds', help='Clouds list', required=True, type=click.File('r'), default='notifyclouds.csv')
@click.option('--config', help='Notify data file', required=True, type=click.File('r'), default='notifyconfig.json')
def send(clouds, config):
    iam_api = IamAPI()
    notify_api = NotifyAPI()
    with config as notify_data, clouds as clouds_list:
        config = json.load(notify_data)
        csv_length = len([x for x in clouds_list if x != '\n']) - 1
        clouds_list.seek(0)
        for num, row in enumerate(csv.DictReader(clouds_list), 1):
            cloud_id = row['cloud_id']
            logger.info(
                '[{}/{}] Working with cloud {}'.format(num, csv_length, cloud_id))
            emails = iam_api.get_email(cloud_id)
            try:
                for email in emails:
                    logger.info('Sending mail to {}'.format(email))
                    notify_api.mail(
                        email, config['data'], config['subject'], config['type'])
            except TypeError:
                logger.warning(
                    'Notify API response: Mail wasnt send cause of IAM API Error')
                continue
    if warnlist:
        logger.warning('Problems reached with emails:\n{}\n recorded info to .log file'.format(warnlist))
        today = datetime.now().strftime('%Y-%m-%d')
        with open('log-' + str(today), 'w+') as notifylog:
            for key, value in warnlist.items():
                notifylog.write('email {} error {}'.format(key, str(value)))


cli.add_command(send)
cli.add_command(init)


if __name__ == "__main__":
    cli()


# [] Exception while message wasnt sended cause of HTTPError (API Timeout :\)
