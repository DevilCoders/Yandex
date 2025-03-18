import logging
import numbers
from typing import Optional

from user_plugins import PluginContainer


class MetricCapsException(Exception):
    pass


# available offline metric capabilities


def has_value(metric_instance):
    return hasattr(metric_instance, "value")


def has_value_by_position(metric_instance):
    return hasattr(metric_instance, "value_by_position")


def has_aggregate_by_position(metric_instance):
    return hasattr(metric_instance, "aggregate_by_position")


def is_regular_metric(metric_instance):
    return has_value(metric_instance)


def is_detailed_metric(metric_instance):
    return (has_value_by_position(metric_instance) and
            has_aggregate_by_position(metric_instance))


def has_precompute(metric_instance):
    return hasattr(metric_instance, "precompute")


def has_serp_depth(metric_instance):
    return hasattr(metric_instance, "serp_depth")


def get_metric_scale_stat_depth(metric_instance):
    if hasattr(metric_instance, "scale_stat_depth"):
        return metric_instance.scale_stat_depth()
    else:
        return None


class ContainerProps(object):
    def __init__(self, has_detailed: bool, max_serp_depth: Optional[int]):
        self.has_detailed = has_detailed
        self.max_serp_depth = max_serp_depth

    def __str__(self):
        return "ContProps(has-detailed: {}, max.depth: {}".format(self.has_detailed, self.max_serp_depth)


def validate_one_metric_capabilities(metric_instance, metric_key: str, detailed_flags: dict,
                                     serp_depths: dict, has_full_depth: dict) -> None:
    detailed_flags[metric_key] = is_detailed_metric(metric_instance)

    if is_regular_metric(metric_instance):
        # check abcence of detailed methods
        has_detailed_methods = (has_value_by_position(metric_instance),
                                has_aggregate_by_position(metric_instance),
                                has_precompute(metric_instance))

        if any(has_detailed_methods):
            raise MetricCapsException("Regular metric {} has '*_by_position' or 'precompute' methods.".format(metric_key))
    elif is_detailed_metric(metric_instance):
        if is_regular_metric(metric_instance):
            raise MetricCapsException("Detailed metric could not contain 'value' method.")
    else:
        raise MetricCapsException("Metric {} is neither regular, nor detailed. ".format(metric_key))

    has_full_depth[metric_key] = False

    if has_serp_depth(metric_instance):
        serp_depth = metric_instance.serp_depth()
        if serp_depth is not None:
            if not isinstance(serp_depth, numbers.Integral):
                raise MetricCapsException("Serp depth for metric {} is not an integer: {}".format(metric_key, serp_depth))
            serp_depths[metric_key] = serp_depth
        else:
            has_full_depth[metric_key] = True
    else:
        has_full_depth[metric_key] = True


def validate_metric_capabilities(plugin_container: PluginContainer, be_verbose=True) -> ContainerProps:
    detailed_flags = {}
    serp_depths = {}
    has_full_depth = {}
    for metric_id, metric_instance in plugin_container.plugin_instances.items():
        metric_key = plugin_container.plugin_key_map[metric_id]
        try:
            logging.info("validating metric %s (%s)", metric_id, metric_key)
            validate_one_metric_capabilities(metric_key=metric_key,
                                             metric_instance=metric_instance,
                                             detailed_flags=detailed_flags,
                                             serp_depths=serp_depths,
                                             has_full_depth=has_full_depth)
        except MetricCapsException as exc:
            logging.error("Metric %s (%s) has incorrect capabilities: %s", metric_id, metric_key, exc)

            if plugin_container.skip_broken_plugins:
                plugin_container.unload_broken_plugin(plugin_id=metric_id, error=str(exc))
            else:
                raise

    if be_verbose:
        show_metric_table(plugin_container=plugin_container, detailed_flags=detailed_flags, serp_depths=serp_depths)

    batch_has_detailed = any(detailed_flags.values())
    batch_has_full_depth = any(has_full_depth.values())

    if serp_depths and not batch_has_full_depth:
        max_serp_depth = max(serp_depths.values())
        logging.info("Max serp depth is set to %d", max_serp_depth)
    else:
        logging.info("Max serp depth is not limited")
        max_serp_depth = None

    return ContainerProps(has_detailed=batch_has_detailed,
                          max_serp_depth=max_serp_depth)


def show_metric_table(plugin_container, detailed_flags, serp_depths):
    table_columns = " m.id : detail : s.depth : metric.alias"
    caps_text = "metric capabilities"
    line_len = (len(table_columns) - len(caps_text)) // 2
    line = "=" * line_len
    table_header = "{} {} {}".format(line, caps_text, line)
    logging.info("%s", table_header)
    logging.info("%s", table_columns)
    for metric_id in plugin_container.plugin_instances:
        metric_key = plugin_container.plugin_key_map[metric_id]
        det_flag = "YES" if detailed_flags[metric_key] else "-"
        serp_depth = serp_depths.get(metric_key, "-")
        logging.info("[%3d] : %6s : %7s : %s", metric_id, det_flag, serp_depth, metric_key)
    logging.info("=" * len(table_header))
