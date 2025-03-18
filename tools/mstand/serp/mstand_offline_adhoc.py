from . import serpset_parser
from . import SerpsetParseContext
from . import PoolParseContext
from . import SerpAttrs
from . import ParsedSerp
from . import OfflineComputationCtx
from . import OfflineMetricCtx
from . import OfflineGlobalCtx
from . import SerpMetricValue

from experiment_pool import Experiment
from experiment_pool import Observation

import yaqutils.file_helpers as ufile
import mstand_metric_helpers.common_metric_helpers as mhelp
from user_plugins import PluginContainer
import serp.serp_compute_metric_single as scm_single


def make_serp_attrs(serp_reqs=None, component_reqs=None, sitelink_reqs=None, judgements=None):
    """
    :type serp_reqs: list[str] | None
    :type component_reqs: list[str] | None
    :type sitelink_reqs: list[str] | None
    :type judgements: list[str] | None
    :rtype:
    """
    return SerpAttrs(serp_reqs=serp_reqs, component_reqs=component_reqs, sitelink_reqs=sitelink_reqs, judgements=judgements)


def load_serpset(serpset_file_name, serp_attrs, allow_no_position=False, allow_broken_components=False):
    """
    :type serpset_file_name: str
    :type serp_attrs: SerpAttrs
    :rtype: list[ParsedSerp]
    """
    pool_ctx = PoolParseContext(serp_attrs=serp_attrs, allow_no_position=allow_no_position, allow_broken_components=allow_broken_components)
    serpset_id = serpset_file_name.replace(".json", "", 1)
    serpset_ctx = SerpsetParseContext(pool_parse_ctx=pool_ctx, serpset_id=serpset_id)
    return serpset_parser.parse_serpset_raw(ctx=serpset_ctx, raw_serpset_file_name=serpset_file_name)


def load_serpset_data(serpset_data, serp_attrs, allow_no_position=False, allow_broken_components=False):
    """
    :type serpset_data: list[dict]
    :type serp_attrs: SerpAttrs
    :rtype: list[ParsedSerp]
    """
    pool_ctx = PoolParseContext(serp_attrs=serp_attrs, allow_no_position=allow_no_position, allow_broken_components=allow_broken_components)
    serpset_id = "1"
    serpset_ctx = SerpsetParseContext(pool_parse_ctx=pool_ctx, serpset_id=serpset_id)
    return serpset_parser.parse_serpset_data(ctx=serpset_ctx, serpset_data=serpset_data)


def load_metric(class_name, module_name, metric_kwargs=None, source=None, metric_alias="adhoc-metric",
                metric_module_dir="mstand_adhoc_metric_module"):
    """
    :type class_name: str
    :type module_name: str
    :type metric_kwargs: dict | None
    :type source: str | None
    :type metric_alias: str
    :type metric_module_dir: str
    :rtype: OfflineMetricCtx
    """
    ufile.make_dirs(metric_module_dir)
    metric_container = PluginContainer.create_single(module_name=module_name,
                                                     class_name=class_name,
                                                     source=source,
                                                     alias=metric_alias,
                                                     kwargs=metric_kwargs,
                                                     skip_broken_plugins=False,
                                                     plugin_dir=metric_module_dir)
    metric_ctx = _make_metric_context(metric_container)
    return metric_ctx


def load_metric_from_command_line(cli_args):
    """
    :type cli_args:
    :rtype: OfflineMetricCtx
    """
    metric_container = mhelp.create_container_from_cli_args(cli_args, skip_broken_metrics=False)
    metric_ctx = _make_metric_context(metric_container)
    return metric_ctx


def compute_metric_on_serp(metric_ctx, parsed_serp):
    """
    :type metric_ctx: OfflineMetricCtx
    :type parsed_serp: ParsedSerp
    :rtype: SerpMetricValue
    """
    experiment = Experiment(serpset_id="fake-adhoc-serpset-id")
    observation = Observation(obs_id=None, control=experiment, dates=None)

    comp_ctx = OfflineComputationCtx(metric_ctx=metric_ctx, observation=observation, experiment=experiment)

    # extract one metric, for simplicity
    serp_metric_values_iter = scm_single.compute_metrics_on_serp(comp_ctx=comp_ctx, parsed_serp=parsed_serp)
    serp_metric_values = list(serp_metric_values_iter)
    return serp_metric_values[0]


def _make_metric_context(metric_container):
    """
    :type metric_container: PluginContainer
    :rtype: OfflineMetricCtx
    """
    global_ctx = OfflineGlobalCtx(be_verbose=False)
    metric_ctx = OfflineMetricCtx(global_ctx=global_ctx, plugin_container=metric_container,
                                  metric_storage=None, parsed_serp_storage=None, be_verbose=False)

    return metric_ctx
