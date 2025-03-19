import logging
import logging.config
import glob
import os.path
from yaml import load
from json import dumps
from pkg_resources import resource_string


class Generator:
    def __init__(self):
        logging.config.dictConfig(load(resource_string(__name__, 'logging.yaml')))
        self.log = logging.getLogger(__class__.__name__)

        self.result = {
            'title': 'Music balancers',
            'type': 'panel',
            'charts': [],
        }

        self.charts = (
            ('success', 'succ'),
            ('fail', 'fail'),
            ('inprogress', 'inprog'),
            ('keepalive', 'ka'),
            ('non-keepalive', 'nka'),
            ('cancelled-sessions', 'cancelled_session'),
            ('dropped', 'dropped'),
        )
        self.requests = tuple(x[1] for x in self.charts)
        self.timelines = (0.05, 0.1, 0.3, 0.5, 0.7, 1, 3, 10)
        self.codes = ('1xx', '2xx', '3xx', '4xx', '404', '5xx')

        self.row = 1

    def add_chart_item(self, col: int, geo: str, prj: str, ctype: str, title: str, template: str, items: tuple):
        chart = {
            'normalize': True,
            'title': '{geo} {title}'.format(geo=geo, title=title),
            'type': 'graphic',
            'width': 1,
            'height': 1,
            'row': self.row,
            'col': col,
            'signals': [],
        }
        if not ctype:
            ctype = 'prestable' if geo == 'sas' else 'prod'
        for item in items:
            signal = {
                'tag': 'balancer_{ctype}_{prj}_{geo}_self'.format(ctype=ctype, prj=prj, geo=geo),
                'host': 'ASEARCH',
                'name': template.format(item),
                'title': str(item)
            }
            chart['signals'].append(signal)
        self.result['charts'].append(chart)

    def add_text(self, text, wide=False):
        self.result['charts'].append({
            'type': 'text',
            'width': 4 if wide else 1,
            'height': 1,
            'row': self.row,
            'col': 1,
            'text': text,
        })

    def add_charts(self, config_file):
        config = load(open(config_file))
        if 'global' not in config:
            return
        conf_global = config['global']
        if 'yasm' not in conf_global or \
           'name' not in conf_global or \
           'project' not in conf_global or conf_global['project'] != 'music':
            return

        balancer_name = conf_global['name']
        yasm_prj = conf_global['yasm']['prj']
        yasm_ctype = conf_global['yasm']['ctype'] if 'ctype' in conf_global['yasm'] else None
        dcs = sorted((x.split('/')[1].split('.')[0] for x in glob.glob(os.path.dirname(config_file) + '/???.*')))
        dcs = (x if x in ('sas', 'man', 'vla') else 'msk' for x in dcs)

        self.add_text(balancer_name, True)
        self.row += 1
        for geo in dcs:
            def chart_add(col, title, template, items):
                return self.add_chart_item(col, geo, yasm_prj, yasm_ctype, title, template, items)

            self.add_text(geo)
            chart_add(2,
                      'requests',
                      'balancer_report-report-service_total-{}_summ',
                      self.requests)
            chart_add(3,
                      'quantiles',
                      'hperc~balancer_report-report-service_total-externalunknown_hgram~0~{}',
                      self.timelines)
            chart_add(4,
                      'codes',
                      'balancer_report-report-service_total-{}-externalunknown_summ',
                      self.codes)
            self.row += 1

    def run(self):
        configs = sorted(glob.glob('*/config.yaml'))
        for config in configs:
            self.add_charts(config)
        print(dumps(self.result, indent=None))
