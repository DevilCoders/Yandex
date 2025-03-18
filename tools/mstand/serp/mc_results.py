import yaqutils.json_helpers as ujson
import yaqutils.six_helpers as usix

from serp import SerpQueryInfo  # noqa
from serp import ExtMetricValue  # noqa


class ExtMetricResultsWriter(object):
    def __init__(self, ext_res_fd, metric_keys, mc_alias_prefix, mc_error_alias_prefix, cache_size=100):
        """
        :type ext_res_fd:
        :type metric_keys: dict[int, PluginKey]
        :type mc_alias_prefix
        :type mc_error_alias_prefix
        :type cache_size: int
        """
        self.ext_res_fd = ext_res_fd
        self.cache_size = cache_size
        self.metric_keys = metric_keys
        assert mc_alias_prefix != mc_error_alias_prefix
        self.mc_alias_prefix = mc_alias_prefix
        self.mc_error_alias_prefix = mc_error_alias_prefix
        self.row_cache = []
        self.first_line = True
        self._open_query_array()

    def _open_query_array(self):
        self.ext_res_fd.write("[\n")

    def write_one_query(self, query_info, serp_depth, ext_metric_vals):
        """
        :type query_info: SerpQueryInfo
        :type serp_depth: int
        :type ext_metric_vals: dict[int, ExtMetricValue]
        :rtype: None
        """
        query_result = self._make_query_result(query_info, serp_depth, ext_metric_vals)
        self.row_cache.append(query_result)
        if len(self.row_cache) > self.cache_size:
            self._flush()

    def _flush(self):
        for line in self.row_cache:
            if self.first_line:
                self.first_line = False
            else:
                self.ext_res_fd.write(",")
            ujson.dump_to_fd(obj=line, fd=self.ext_res_fd, pretty=True, sort_keys=True)
        self.row_cache = []

    def _close_query_array(self):
        self.ext_res_fd.write("]\n")

    def terminate(self):
        self._flush()
        self._close_query_array()

    def _make_query_result(self, query_info, serp_depth, ext_metric_vals):
        """
        :type query_info: SerpQueryInfo
        :type serp_depth: int
        :type ext_metric_vals: dict[int, ExtMetricValue]
        :rtype: dict
        """
        query_key_external = query_info.query_key.serialize_external()
        query_result = {"query": query_key_external}

        pos_results = [{"type": "COMPONENT"} for _ in usix.xrange(serp_depth)]

        has_detailed_metrics = False

        for metric_id, ext_metric_val in usix.iteritems(ext_metric_vals):

            metric_alias = self.metric_keys[metric_id].str_key()
            ext_metric_key = "{}{}".format(self.mc_alias_prefix, metric_alias)
            query_result[ext_metric_key] = ext_metric_val.total_value
            if ext_metric_val.has_error:
                error_key = "{}{}".format(self.mc_error_alias_prefix, metric_alias)
                query_result[error_key] = ext_metric_val.error_message

            if ext_metric_val.pos_values is not None:
                has_detailed_metrics = True
                for index, pos_value in enumerate(ext_metric_val.pos_values):
                    pos_results[index][ext_metric_key] = pos_value

        if has_detailed_metrics:
            query_result["components"] = pos_results
        return query_result
