from copy import deepcopy
from datetime import datetime, timedelta

from . import DataSource

from yql.api.v1.client import YqlClient


class DSYql(DataSource):
    """
    Provides object to get data from YQL by query
    """
    QUERY_SERVICES_MACROS = '__ANTIADB_SERVICE_IDS__'

    def __init__(self, service_ids, config, logger, oauth_token):
        super(DSYql, self).__init__(service_ids, config, logger)
        self.query = self.args['query'].replace(self.QUERY_SERVICES_MACROS,
                                                ','.join(map(lambda s: "'{}'".format(s), service_ids)))
        if self.args.get('placeholders') is not None:
            self.query = self.query.format(**(self.args['placeholders']))
        self.client = YqlClient(db='hahn', token=oauth_token)

    def get_check_result(self):
        query = 'PRAGMA yt.Pool="antiadb";\n' + self.query
        request = self.client.query(query, syntax_version=1, title="monrelay dsyql check_id={} YQL".format(self.config.get('check_id')))
        request.run()
        result = list()
        for table in request.get_results():
            table.fetch_full_data()
            for row in table.rows:
                if str(row[0]) in self.service_ids:
                    subresult = deepcopy(self.config)
                    now = datetime.utcnow()
                    subresult.update(
                        {
                            'service_id': str(row[0]),
                            'value': str(row[1]),
                            'state': str(row[2]),
                            'last_update': now.strftime(self.datetime_format),
                            'valid_till': (now + timedelta(seconds=subresult.pop('ttl'))).strftime(self.datetime_format),
                            'external_url': request.share_url
                        }
                    )
                    result.append(subresult)
        return result
