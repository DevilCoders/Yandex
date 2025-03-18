
class MetricsClient(object):
    """
    Simple client to get data from any source
    """

    def date_histogram_splitted_by_one_field(self, service_id, date_gt, filter2=None, date_lt='now', histogram_interval='60s', query_filter='*'):
        """
        Get grouped by date and one field data from elasticsearch like Kibana Visuzalizations.
        :param service_id: service id
        :param date_gt: `select from` date at timestamp millis format
        :param date_lt: `select to` date at timestamp millis format. Default: now.
        :param histogram_interval: time histogram interval to group selected data at elasticsearch format (1m, 1h, 1d)
        :param query_filter: Additional query filter expression in kibana search form format like 'http_code:[400 TO 599]'
        :return: dict with grouped by time and field data
        {timestamp1: {field1: 123, field2: 456, ...}, timestamp2: {field1: 789, field2: 987, ...},}
        """

        return {}

    def date_histogram_splitted_by_two_fields(self, service_id, date_gt, date_lt='now', histogram_interval='60s'):
        """
        Get grouped by date and two fields data from elasticsearch like Kibana Visuzalizations with double splitting (bamboozled by adb app for example).
        :param service_id: service id
        :param date_gt: `select from` date at timestamp millis format
        :param date_lt: `select to` date at timestamp millis format. Default: now.
        :param histogram_interval: time histogram interval to group selected data at elasticsearch format (1m, 1h, 1d)
        :return: dict with grouped by time and field data
        {timestamp1: {parent_aggr_field1: {child_aggr_field1: 123, child_aggr_field2: 456}, ...},...}}
        """
        return {}

    def date_histogram_action_timings_percentiles(self, percents, service_id, action, date_gt, date_lt='now', histogram_interval='60s'):
        """
        Get grouped by date action timings percentiles for field from elasticsearch
        :param percents: list with percents to calculate percentiles: (90, 95, 98)
        :param service_id: service id
        :param action: action to measure timings
        :param date_gt: `select from` date at timestamp millis format
        :param date_lt: `select to` date at timestamp millis format. Default: now.
        :param histogram_interval: time histogram interval to group selected data at elasticsearch format (1m, 1h, 1d)
        :return: dict with grouped by time and percent percentiles in milliseconds
        {timestamp1: {percent1: 123, percent2: 456, ...}, timestamp2: {percent1: 789, percent2: 478, ...},}
        """
        return {}
