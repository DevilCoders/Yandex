from yaqschemas import SchemaBuilder


# FIXME: poor depends design
# from metrics_api.offline import SerpMetricValuesForAPI


# compact structure for external output
class ExtMetricValue:
    def __init__(self, one_metric_val):
        """
        :type one_metric_val: SerpMetricValuesForAPI
        """
        self.total_value = one_metric_val.total_value
        # store numbers only
        if one_metric_val.has_details:
            self.pos_values = [pv.value for pv in one_metric_val.values_by_position]
        else:
            self.pos_values = None
        self.has_error = one_metric_val.has_error
        self.error_message = one_metric_val.error_message


class SerpsetMetricValues(object):
    def __init__(self, use_int_vals):
        # query_ext_metric_values are rewritten for each query
        self.query_ext_metric_vals = {}
        self.metric_vals = {} if use_int_vals else None

    def add_metric_value(self, metric_id, qid, one_metric_val):
        """
        :type metric_id: int
        :type qid: int
        :type one_metric_val: SerpMetricValuesForAPI
        :rtype: None
        """
        self.query_ext_metric_vals[metric_id] = ExtMetricValue(one_metric_val)

        if self.metric_vals is None:
            return
        if one_metric_val.total_value is not None:
            if metric_id not in self.metric_vals:
                self.metric_vals[metric_id] = {}
            self.metric_vals[metric_id][qid] = one_metric_val.total_value


class SerpMetricValue:
    def __init__(self, qid, metric_id, one_metric_val):
        """
        :type qid: int
        :type metric_id: int | None
        :type one_metric_val: SerpMetricValuesForAPI
        """
        self.qid = qid
        self.metric_id = metric_id
        self.one_metric_val = one_metric_val

    def get_total_value(self):
        """
        :rtype: float
        """
        return self.one_metric_val.total_value

    def get_values_by_position(self):
        """
        :rtype: list[float]
        """
        return [vbp.value for vbp in self.one_metric_val.values_by_position]

    def __str__(self):
        return "SMV[qid={} mid={}: total={}]".format(self.qid, self.metric_id, self.one_metric_val.total_value)


class ExtMetricResultsTable:
    class Fields:
        SERPSET_ID = "serpset-id"
        METRIC = "metric"
        QUERY = "query"
        TOTAL_VALUE = "total-value"
        VALUES_BY_POSITION = "values-by-position"
        HAS_ERROR = "has-error"
        ERROR_MESSAGE = "error-message"
        MSTAND_QID = "mstand-qid"
        MSTAND_METRIC_ID = "mstand-metric-id"

    @classmethod
    def get_offline_metric_yt_output_table_schema(cls):
        """
        :rtype: list
        """
        schema_builder = SchemaBuilder()
        schema_builder.add_string_column(cls.Fields.SERPSET_ID)
        schema_builder.add_string_column(cls.Fields.METRIC)
        schema_builder.add_any_column(cls.Fields.QUERY)
        schema_builder.add_any_column(cls.Fields.TOTAL_VALUE)
        schema_builder.add_any_column(cls.Fields.VALUES_BY_POSITION)
        schema_builder.add_boolean_column(cls.Fields.HAS_ERROR)
        schema_builder.add_string_column(cls.Fields.ERROR_MESSAGE)
        schema_builder.add_int64_column(cls.Fields.MSTAND_QID)
        schema_builder.add_int64_column(cls.Fields.MSTAND_METRIC_ID)
        return schema_builder.get_schema()
