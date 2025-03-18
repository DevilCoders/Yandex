import os

from experiment_pool import MetricColoring
from experiment_pool import MetricValueType

from user_plugins import PluginContainer


def create_container_from_cli_args(cli_args, lamps_mode=False, skip_broken_metrics=False):
    batch_file = cli_args.batch
    if lamps_mode:
        if not cli_args.batch:
            raise Exception("Lamps mode specified, but lamp batch file is not.")

        source = os.path.dirname(batch_file)
    else:
        source = cli_args.source

    metric_cont = PluginContainer.create_from_command_line(cli_args, batch_file=batch_file,
                                                           source=source, skip_broken_plugins=skip_broken_metrics)

    for metric_instance in metric_cont.plugin_instances.values():
        MetricColoring.set_metric_instance_coloring(metric_instance, cli_args.set_coloring)
        MetricValueType.set_metric_instance_value_type(metric_instance)

    return metric_cont


# actually not used anywhere
def generate_output_file_names(metric_container, output_file_name):
    """
    :type metric_container: PluginContainer
    :type output_file_name: str | None
    :rtype: dict[int, str]
    """
    if not output_file_name:
        return None

    metrics = metric_container.plugin_instances

    if not metrics:
        return None
    if len(metrics) == 1:
        return {metrics.keys()[0]: output_file_name}

    results = {}
    for metric_id in metric_container.plugin_instances.keys():
        filename, ext = os.path.splitext(output_file_name)
        if ext:
            batch_file_name = "{}_{}.{}".format(filename, metric_id, ext)
        else:
            batch_file_name = "{}_{}".format(filename, metric_id)
        results[metric_id] = batch_file_name
    return results
