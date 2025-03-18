#!/usr/bin/env python3
# coding=utf-8

import logging
from typing import Dict
from typing import Optional

import yaqutils.misc_helpers as umisc

import blender_slices
import metric_confidences
from metrics_tools import WizardFilter
from mstand_enums.mstand_offline_enums import ModesEnum
from omglib import MetricDescription
from omglib import mc_common
from styles import df_style
from wizard_filters import OTHER
from wizard_filters import WIZARD_FILTERS

REVISION = 9609550
RESPONSIBLE_USERS = ["robot-uno", "xetd71", "annapershina"]

all_snipet_plugins = set()
for plugins in blender_slices.blender_snippets.values():
    all_snipet_plugins.update(plugins)


def create_metric(
        mode: str,
        prefix: str,
        metric_name: str,
        wizard_filter: Optional[WizardFilter] = None,
        metric_class: str = "DefectRate",
        metric_module: str = "defect_rate",
        comment: str = "",
        description: Optional[str] = None,
        additional_kwargs: Optional[dict] = None,
):
    assert mode in ModesEnum.ALL, "unknown mode"

    metric_name = "{}{}".format(prefix + (metric_name and "-"), metric_name)
    kwargs = {
        "max_depth": 30,
        "mode": mode,
        "judged": True,
    }
    if wizard_filter is not None:
        kwargs.update(wizard_filter.get_kwargs())
    if additional_kwargs is not None:
        kwargs.update(additional_kwargs)

    requirements = {
        "COMPONENT.judgements.serp_irrel_bad_count",
        "COMPONENT.judgements.serp_irrel_comments",
        "COMPONENT.judgements.element_screen_url",
        "COMPONENT.judgements.full_serp_html_url",
    }
    if wizard_filter is not None:
        requirements.update(wizard_filter.get_requirements())

    confidences = metric_confidences.get_defect_rate_confidences()

    if description is None:
        description = u"{} {} {}".format(prefix, mode, comment)

    return MetricDescription(
        name=metric_name,
        owner="robot-uno",
        description=description,
        revision=REVISION,
        url="svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia/quality/mstand_metrics/offline_production/metrics",
        metric_module=metric_module,
        metric_class=metric_class,
        kwargs=kwargs,
        requirements=requirements,
        confidences=confidences,
        use_py3=True,
        greater_better=False,
        responsible_users=RESPONSIBLE_USERS,
        style=df_style
    )


def generate_metrics_with_params(prefix: str, suffix: str, kwargs: Dict):
    metrics = []
    for name, wizard_filters in WIZARD_FILTERS.items():
        if isinstance(wizard_filters, dict):
            wizard_filter = WizardFilter.merge_filters(list(wizard_filters.values()))
            for sub_name, sub_wizard_filter in wizard_filters.items():
                if sub_name != OTHER:
                    metrics.append(create_metric(
                        "avg", prefix, f"{sub_name}{suffix}", sub_wizard_filter,
                        additional_kwargs=kwargs,
                    ))
        else:
            wizard_filter = wizard_filters
        metrics.append(create_metric(
            "avg", prefix, f"{name}{suffix}", wizard_filter,
            additional_kwargs=kwargs,
        ))

    return metrics


def generate_metrics(prefix: str = "defect_rate"):
    metrics = []
    for depth in [5, 10]:
        metrics.extend(generate_metrics_with_params(prefix, f"-depth-{depth}", {"th": 4, "depth": depth}))
    return metrics


def main():
    cli_args = mc_common.parse_args(default_dir="defect_rate")
    umisc.configure_logger()

    logging.info("Generating Defect Rate metrics")
    metrics = generate_metrics()
    mc_common.save_generated(metrics, cli_args)


if __name__ == "__main__":
    import os

    os.chdir("metrics_tools")
    main()
