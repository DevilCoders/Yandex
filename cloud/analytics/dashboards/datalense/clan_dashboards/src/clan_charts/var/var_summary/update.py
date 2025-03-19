import requests
import json
from clan_charts.var.var_summary.queries import accounts_count, by_accounts, consumption_by_period, sub_query, subaccounts_count_vs_total_cons
from clan_tools.data_adapters.ChartsAdapter import ChartsAdapter
from clan_tools.datalense.ChartsDir import ChartsDir
from clan_charts.var.var_summary.templates.url_template import url
import logging.config
from clan_tools.utils.conf import read_conf

logging.config.dictConfig(read_conf('config/logger.yml'))


if __name__ == '__main__':
   

    charts_adapter = ChartsAdapter()
    common_dir = ChartsDir('./var_summary/charts/common')


    charts_dir = ChartsDir('./var_summary/charts/total_consumption_vs_total_subaccounts_count')
    charts_adapter.update_chart('74djg7pbvc702',
     {
        "data": {
            "js": charts_dir.read('js.js'),
            "url": url(query = consumption_by_period + sub_query + by_accounts + subaccounts_count_vs_total_cons),
            "graph": charts_dir.read('graph.js'),
            'params': common_dir.read('params.js'),
            'statface_graph' : common_dir.read('statface_graph.js'),
            'ui' :  common_dir.read('ui.js'),
            'shared': common_dir.read('shared.js')},
        "key": "VAR/VAR Summary/total_consumption_vs_total_subaccounts_count",
        "type": "graph_node"
    })

    charts_dir = ChartsDir('./var_summary/charts/accounts_count')
    charts_adapter.update_chart('todckp4s5s5oo',
     {
        "data": {
            "js": charts_dir.read('js.js'),
            "url": url(query = consumption_by_period + sub_query + accounts_count),
            "graph": charts_dir.read('graph.js'),
            'params': common_dir.read('params.js'),
            'statface_graph' : common_dir.read('statface_graph.js'),
            'ui' :  common_dir.read('ui.js'),
            'shared': common_dir.read('shared.js')},
        "key": "VAR/VAR Summary/accounts_count",
        "type": "graph_node"
    })
