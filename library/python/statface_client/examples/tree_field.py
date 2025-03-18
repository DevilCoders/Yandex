#!/usr/bin/env python
# coding: utf-8
# pylint: disable=invalid-name

from statface_client import StatfaceClient
from statface_client.report import StatfaceReportConfig

client = StatfaceClient()
report = client.get_report('Adhoc/Adhoc/Examples/TreeField')
config = StatfaceReportConfig(
    title=u'Заливка древесных данных работает',
    dimensions=[('fielddate', 'date'), ('zuzuzu', 'tree')],
    measures=[('count', 'number')])
report.upload_config(config)
big_data = [dict(fielddate='2116-03-17', zuzuzu='\tA\tB\tC\t', count=7),
            dict(fielddate='2116-03-17', zuzuzu='\tA\t', count=42),
            dict(fielddate='2116-03-18', zuzuzu=['D', 'E', 'F'], count=1)]
report.upload_data(scale='daily', data=big_data)
