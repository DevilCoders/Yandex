import string

import yaqutils.six_helpers as usix

from user_plugins import PluginContainer
from user_plugins import PluginKey


def is_valid_custom_file_name(file_name):
    return set(file_name) <= set(string.ascii_letters + string.digits + ".-_=[]{}()@")


def is_pool_processor(postproc_items):
    return hasattr(postproc_items, "process_pool")


def is_observation_processor(postproc_items):
    return hasattr(postproc_items, "process_observation")


def is_experiment_processor(postproc_items):
    return hasattr(postproc_items, "process_experiment")


def validate_processor(pp_instance):
    pool_pp = is_pool_processor(pp_instance)
    obs_pp = is_observation_processor(pp_instance)
    exp_pp = is_experiment_processor(pp_instance)
    pp_types = [pool_pp, obs_pp, exp_pp].count(True)
    if pp_types > 1:
        raise Exception("Ambigous postprocessor type.")

    if pp_types == 0:
        raise Exception("No process_{experiment,observation,pool} method in postprocessor.")


def validate_proces_container(postprocess_container):
    """
    :type postprocess_container: user_plugins.PluginContainer
    """
    for pp_id, pp_instance in usix.iteritems(postprocess_container.plugin_instances):
        validate_processor(pp_instance)


def make_metric_key(metric_key, pp_key):
    """
    :type metric_key: PluginKey
    :type pp_key: PluginKey
    :rtype: PluginKey
    """
    new_name = "{}-{}".format(metric_key.name, pp_key.str_key())
    return PluginKey(new_name, metric_key.kwargs_name)


def create_postprocessor_from_cli(cli_args):
    batch_file = cli_args.batch
    source = cli_args.source
    postprocessor = PluginContainer.create_from_command_line(cli_args, batch_file=batch_file, source=source)
    validate_proces_container(postprocessor)
    return postprocessor
