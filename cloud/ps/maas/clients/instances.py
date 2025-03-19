import requests
import logging
import os
import config
import json
import uuid

'''
This module is for actualising customers instances list for monitoring
YC utility should be installed on the host to run module properly
Please read comments below
'''

logger = logging.getLogger('ClientsInstancesLogger')
logger.setLevel(logging.DEBUG)
f = logging.FileHandler('main.log')
formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
f.setFormatter(formatter)
logger.addHandler(f)

'''
Entry point. You need to send CloudID to get actual instances list
TODO: need function to get cloudID from customer ID
'''
class Instances():

    def __init__(self):
        logger.info('Instances called')
        self.authHeader = {'X-YaCloud-SubjectToken':config.API_TOKEN, 'Content-Type':'application/json'}

    def getFolderList(self, cloudID: str):
        logger.debug('Trying to get list of all folders for cloud: ' + cloudID)
        urls = config.FOLDERS_URL + cloudID
        uid = str(uuid.uuid4())
        logger.debug('UUID for request is: ' + uid)
        header = self.authHeader
        header['X-Request-ID'] = uid
        try:
            res = requests.get(urls, headers = header, timeout=10)
        except Exception as e:
            logger.error('Error occured: ' + str(e))
            return None
        if res.status_code != 200:
            logger.error('Private API returned: ' + str(res.status_code) + ' Answer: ' + res.text)
            return None
        lister = json.loads(res.text)
        lst = []
        for i in range (0, len(lister['result'])):
            lst.append(lister['result'][i]['id'])
        return lst

    def getComputeInstancesFromFolder(self, cloudID: str):
        folderList = self.getFolderList(cloudID)
        logger.debug('Trying to get list of all compute instances in folders: ' + str(folderList))
        header = self.authHeader
        nlist = []
        for f in folderList:
            urls = config.INSTANCES_URL + f
            uid = str(uuid.uuid4())
            logger.debug('UUID for request is: ' + uid)
            header['X-Request-ID'] = uid
            try:
                res = requests.get(urls, headers = header, timeout=10)
            except Exception as e:
                logger.error('Error occured: ' + str(e))
                return None
            if res.status_code != 200:
                logger.error('Private API returned: ' + str(res.status_code) + ' Answer: ' + res.text)
                return None
            nodes = json.loads(res.text)
            for i in range(0, len(nodes['instances'])):
                nlist.append(nodes['instances'][i]['id'])
        return nlist
        
