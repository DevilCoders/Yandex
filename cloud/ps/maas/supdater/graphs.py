import requests
import json
import re
import logging
import os
import config
import time


'''
Module to work with solomon graphs
'''

logger = logging.getLogger('SupdaterGraphsLogger')
logger.setLevel(logging.DEBUG)
f = logging.FileHandler('main.log')
formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
f.setFormatter(formatter)
logger.addHandler(f)

class Graphs():

    def __init__(self):
        logger.info('Graphs called')
        self.authHeader = {'Authorization':'OAuth ' + config.SOLOMON_TOKEN, 'Content-Type':'application/json', 'Accept':'application/json'}
        os.environ['REQUESTS_CA_BUNDLE'] = ''

    def getGraphFromSolomon(self, graphID: str):
        logger.debug('Trying to get graph from Solomon')
        urls = config.GRAPH_URL + graphID
        logger.debug('We need to get graph: ' + graphID)
        try:
            res = requests.get(urls, headers=self.authHeader, timeout=10)
        except Exception as e:
            logger.error('Error ocured requesting graph from Solomon: ' + str(e))
            return None
        if res.status_code != 200:
            logger.error('Solomon returned: ' + str(res.status_code) + ' Answer: ' + res.text)
            return None
        return json.loads(res.text)
    
    def putGraphToSolomon(self, graphID: str, graphData: str):
        logger.debug('Trying to put graph to Solomon')
        urls = config.GRAPH_URL + graphID
        logger.debug('Will try to put info into graph: ' + graphID + '. This instances should be added: ' + str(graphData))
        try:
            res = requests.put(urls, json=graphData, headers=self.authHeader, timeout=10)
        except Exception as e:
            logger.error('Error ocured during request to Solomon: ' + str(e))
            return None
        if res.status_code != 200:
            logger.error('Solomon returned: ' + str(res.status_code) + ' Answer: ' + res.text)
            return None
        return res.text
    
    def postNewGraphToSolomon(self, graphData: str):
        logger.debug('Trying to post new graph to Solomon')
        try:
            res = requests.post(config.GRAPH_URL, json=graphData, headers=self.authHeader, timeout=10)
        except Exception as e:
            logger.error('Error occured during posting new graph: ' + str(e))
            return None
        if res.status_code != 200:
            logger.error('Solomon returned: ' + str(res.status_code) + ' Answer: ' + res.text)
            return None
        return res.text
    
    def getGraphInstances(self, graphID: str):
        logger.debug('Trying to get instances from loaded graph')
        jdict = self.getGraphFromSolomon(graphID)
        try:
            inst = re.search(r'instance_id=\"\S*\"', str(jdict)).group(0).replace('instance_id=\"', '').replace('\"', '').split('|')
        except Exception as e:
            logger.error('Error occured: ' + str(e) + '. Data: ' + jdict)
            return None
        return inst
    
    def changeGraphInstances(self, graphID: str, instances: list):
        logger.debug('Configuring list with new instances for graph: ' + graphID)
        jdict = self.getGraphFromSolomon(graphID)
        if not jdict:
            logger.error('Something went wrong. getGraphFromSolomon returned None')
            return None
        newList = 'instance_id="' + '|'.join(instances) + '"'
        try:
            change = jdict['elements'][0]['expression']
        except Exception as e:
            logger.error('Error getting instances from Solomon answer: ' + str(e) + '. Problem in: ' + jdict)
            return None
        res = re.sub(r'instance_id=\"\S*\"', newList, change)
        jdict['elements'][0]['expression'] = res
        update = self.putGraphToSolomon(graphID, jdict)
        if not update:
            logger.error('Something went wrong. putGraphToSolomon returned None')
            return None
        logger.debug('Graph ' + graphID + ' updated successfully')
        return update
    
    def createNewGraphFromTemplate(self, metricType: str, instances: list, customerName: str):
        logger.debug('Creating new graph from template. Metric: ' + metricType + '. Customer: ' + customerName)
        newGraphID = customerName + '_' + metricType + '_' + time.strftime('%Y%m%d%H%M%S', time.localtime())
        if metricType == 'flows':
            if not os.path.isfile('gtemplates/flows.json'):
                logger.error('Unable to find template file fo flows metric.')
                return None
            with open('gtemplates/flows.json') as f:
                template = json.load(f)
        elif metricType == 'cpu':
            if not os.path.isfile('gtemplates/cpu.json'):
                logger.error('Unable to find template file fo cpu metric.')
                return None
            with open('gtemplates/cpu.json') as f:
                template = json.load(f)
        template['id'] = newGraphID
        template['name'] = newGraphID
        template['description'] = 'Flows Heatmap for ' + customerName
        insList = 'instance_id="' + '|'.join(instances) + '"'
        template['elements'][0]['expression'] = re.sub(r'instance_id=\"\"', insList, template['elements'][0]['expression'])
        #postNew = self.postNewGraphToSolomon(template)
        return newGraphID
