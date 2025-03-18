from collections import defaultdict
import yaqutils.six_helpers as usix
import logging


# These classes are for used/missing scales (judgements) info collection.
class LevelOneScaleStats(object):
    def __init__(self):
        self.missing = set()
        self.used = set()

    def check_scale(self, scale_name, scales):
        """
        :type scale_name: str
        :type scales: dict
        :rtype: bool
        """
        scale_found = True
        if scale_name not in scales or (scale_name == "RELEVANCE" and scales[scale_name] == "NOT_JUDGED"):
            self.missing.add(scale_name)
            scale_found = False
        self.used.add(scale_name)
        return scale_found


class LevelTwoScaleStats(object):
    def __init__(self):
        self.missing = defaultdict(int)
        self.used = defaultdict(int)

    def update_stats(self, level_one_stats):
        """
        :type level_one_stats: LevelOneScaleStats
        :rtype:
        """
        for scale_name in level_one_stats.missing:
            self.missing[scale_name] += 1

        for scale_name in level_one_stats.used:
            self.used[scale_name] += 1


# actually it's level 3 counters
class TotalScaleStats(object):
    def __init__(self):
        self.missing = {}
        self.used = {}

    def update_stats(self, metric_id, level_two_stats):
        """
        :type metric_id: int
        :type level_two_stats: LevelTwoScaleStats
        :return:
        """
        if level_two_stats is None:
            return

        if metric_id not in self.missing:
            self.missing[metric_id] = defaultdict(int)

        for scale_name, missing_count in usix.iteritems(level_two_stats.missing):
            self.missing[metric_id][scale_name] += missing_count

        if metric_id not in self.used:
            self.used[metric_id] = defaultdict(int)

        for scale_name, used_count in usix.iteritems(level_two_stats.used):
            self.used[metric_id][scale_name] += used_count

    def log_stats(self, header, query_count):
        """
        :type header: str
        :type query_count: int
        :rtype: None
        """
        if not self.used:
            return
        stat_messages = [header]
        for metric_id, used_stats in usix.iteritems(self.used):
            if not used_stats:
                stat_messages.append("  No stats for metric {}".format(metric_id))
            else:
                stat_messages.append("  Stats for metric {}:".format(metric_id))
            for scale_name, used_count in usix.iteritems(used_stats):
                missing_count = self.missing[metric_id][scale_name]
                msg = self.get_one_scale_info_stats(scale_name=scale_name, used_count=used_count,
                                                    missing_count=missing_count, query_count=query_count)
                stat_messages.append(msg)
        logging.info("\n".join(stat_messages))

    # noinspection PyMethodMayBeStatic
    def get_one_scale_info_stats(self, scale_name, used_count, missing_count, query_count):
        hits = used_count - missing_count
        hit_ratio = float(100 * hits) / used_count if used_count else 0.0
        query_ratio = float(hits) / query_count if query_count else 0.0
        status = "[!]" if hit_ratio <= 20.0 else ""
        return "    {:3s} scale {}: hit {:d} of {} ({:1.2f}%), avg. {:1.2f} per query".format(status, scale_name,
                                                                                              hits, used_count,
                                                                                              hit_ratio, query_ratio)


class ScaleStats(object):
    def __init__(self):
        self.total_scale_stats = TotalScaleStats()
        self.total_serp_data_stats = TotalScaleStats()

    def log_stats(self, serpset_id, query_count):
        self.total_scale_stats.log_stats("Result scales stats on serpset {}:".format(serpset_id), query_count)
        self.total_serp_data_stats.log_stats("SERP data fields stats on serpset {}:".format(serpset_id), query_count)

    def update_stats(self, metric_id, one_metric_val):
        self.total_scale_stats.update_stats(metric_id, one_metric_val.scale_stats)
        self.total_serp_data_stats.update_stats(metric_id, one_metric_val.serp_data_stats)
