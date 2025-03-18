#!/usr/bin/env python3
# coding=utf-8

import logging
from functools import partial
from typing import List
from typing import Optional
from typing import Set

import yaqutils.misc_helpers as umisc

import blender_slices
import metric_confidences
from metrics_tools import WizardFilter
from mstand_enums.mstand_offline_enums import ModesEnum
from omglib import MetricDescription
from omglib import mc_common
from wizard_filters import OTHER
from wizard_filters import WIZARD_FILTERS

REVISION = 9609550
RESPONSIBLE_USERS = ["robot-uno", "xetd71", "rubik303", "serg-v", "annapershina"]

all_snipet_plugins = set()
for plugins in blender_slices.blender_snippets.values():
    all_snipet_plugins.update(plugins)


def create_metric(
        name: str,
        mode: str,
        prefix: str,
        metric_class: str,
        metric_module: str,
        confidences: List[dict],
        requirements: Optional[Set[str]] = None,
        comment: str = "",
        depth: int = 30,
        description: Optional[str] = None,
        greater_better: bool = True,
        metric_name: Optional[str] = None,
        add_right_column: bool = True,
        additional_kwargs: Optional[dict] = None,
        wizard_filter: Optional[WizardFilter] = None
):
    assert mode in ModesEnum.ALL, "unknown mode"

    if metric_name is None:
        metric_name = "{}{}{}".format(prefix + (prefix and "-"), mode + (mode and "-"), name)
    kwargs = {
        "depth": depth,
        "max_depth": 30,
    }
    if not add_right_column:
        kwargs["add_right_column"] = False
    if additional_kwargs is not None:
        kwargs.update(additional_kwargs)
    if wizard_filter is not None:
        kwargs.update(wizard_filter.get_kwargs())

    if mode == ModesEnum.COVERAGE:
        kwargs["judged"] = False
        metric_module = "coverage"
        metric_class = "Coverage"
        description = u"{} {} for {} {}".format("uno", mode, name, comment)
    else:
        kwargs["mode"] = mode

    if mode == ModesEnum.AVG or "defect-rate-judged" in metric_name:
        kwargs["judged"] = True
    elif "Impact" not in metric_class:
        kwargs["judged"] = False

    requirements = requirements or set()
    if wizard_filter is not None:
        requirements.update(wizard_filter.get_requirements())

    confidences = (confidences or []) + metric_confidences.get_serp_confidences()

    if description is None:
        description = u"{} {} for {} {}".format(prefix, mode, name, comment)

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
        greater_better=greater_better,
        responsible_users=RESPONSIBLE_USERS,
    )


def create_look_metric(
        name,
        mode,
        wizard_filter: Optional[WizardFilter] = None,
        depth=30,
        prefix="look",
        metric_class="LookV3",
        metric_module="look",
        requirements: Optional[Set[str]] = None,
        comment="\nhttps://st.yandex-team.ru/OFFLINE-921",
        description=None,
        greater_better=True,
        metric_name=None,
        add_right_column=True,
        additional_kwargs: Optional[dict] = None,
        look_enrichment_key: str = "look"
):
    additional_kwargs = additional_kwargs or {}
    if metric_class == "LookV3" and look_enrichment_key != "look" and mode != ModesEnum.COVERAGE:
        additional_kwargs["scale"] = f"{look_enrichment_key}_scores"

    requirements = requirements or set()
    confidences = None
    if mode != ModesEnum.COVERAGE:
        confidences = metric_confidences.get_look_confidences(depth, look_enrichment_key)
        requirements.update({
            f"COMPONENT.judgements.{look_enrichment_key}_scores",
            f"COMPONENT.judgements.element_screen_url",
        })

    return create_metric(
        name=name,
        mode=mode,
        prefix=prefix,
        metric_class=metric_class,
        metric_module=metric_module,
        requirements=requirements,
        confidences=confidences,
        comment=comment,
        depth=depth,
        description=description,
        greater_better=greater_better,
        metric_name=metric_name,
        add_right_column=add_right_column,
        wizard_filter=wizard_filter,
        additional_kwargs=additional_kwargs,
    )


def create_look_height_metric(
        name,
        mode,
        wizard_filter: Optional[WizardFilter] = None,
        depth=30,
        prefix="look-height-v3",
        metric_class="UnoV3",
        metric_module="uno_dev",
        comment="\nhttps://st.yandex-team.ru/OFFLINE-921",
        description=None,
        greater_better=True,
        metric_name=None,
        add_right_column=True,
        look_enrichment_key: str = "look",
        additional_kwargs: Optional[dict] = None,
):
    additional_kwargs = additional_kwargs or {}
    additional_kwargs.update({
        "alpha": 1.0,
        "swap_coeff": 0.0
    })
    requirements = {f"COMPONENT.judgements.element_screen_url"}
    if look_enrichment_key != "look":
        additional_kwargs["look_scale"] = f"{look_enrichment_key}_scores"
        additional_kwargs["look_height_scale"] = f"{look_enrichment_key}_screenshot_height"
    return create_look_metric(
        name=name,
        mode=mode,
        wizard_filter=wizard_filter,
        depth=depth,
        prefix=prefix,
        metric_class=metric_class,
        metric_module=metric_module,
        requirements=requirements,
        comment=comment,
        description=description,
        greater_better=greater_better,
        metric_name=metric_name,
        add_right_column=add_right_column,
        additional_kwargs=additional_kwargs,
        look_enrichment_key=look_enrichment_key,
    )


def create_uno_metric(
        name,
        mode,
        wizard_filter: Optional[WizardFilter] = None,
        depth=30,
        prefix="uno-v3",
        metric_class="UnoV3",
        metric_module="uno_dev",
        comment="\nhttps://st.yandex-team.ru/OFFLINE-1642",
        description=None,
        greater_better=True,
        metric_name=None,
        add_right_column=True,
        use_const_weights=False,
        uno_coeff: Optional[float] = None,
        swap_coeff: Optional[float] = None,
        height_coeff_left_column: Optional[float] = None,
        height_coeff_right_column: Optional[float] = None,
        look_enrichment_key: str = "look",
):
    kwargs = {}
    if uno_coeff is not None:
        kwargs["uno_coeff"] = uno_coeff
    if swap_coeff is not None:
        kwargs["swap_coeff"] = swap_coeff
    if height_coeff_left_column is not None:
        kwargs["height_coeff_left_column"] = height_coeff_left_column
    if height_coeff_right_column is not None:
        kwargs["height_coeff_right_column"] = height_coeff_right_column
    if look_enrichment_key != "look":
        kwargs["look_scale"] = f"{look_enrichment_key}_scores"
        kwargs["look_height_scale"] = f"{look_enrichment_key}_screenshot_height"
    if use_const_weights:
        kwargs["use_const_weights"] = True

    requirements = {
        "COMPONENT.judgements.comb_host_signal_enrichment_2021",
        "COMPONENT.judgements.dup_fine_2021",
        "COMPONENT.judgements.plag_fine_2021",
        "COMPONENT.judgements.sinsig_kc_no_turbo_judgement_values",
        "COMPONENT.judgements.sinsig_kc_no_turbo_slider_values",
        "COMPONENT.judgements.super_dup_only_fine_2021",
        "COMPONENT.judgements.super_text_dup_fine_2021",
        "COMPONENT.judgements.pq_grade_enrichment",
        "SERP.query_param.cluster",
        "SERP.query_param.need_certain_host",
        f"COMPONENT.judgements.{look_enrichment_key}_screenshot_height",
        f"COMPONENT.judgements.{look_enrichment_key}_scores",
        f"COMPONENT.judgements.element_screen_url",
    }
    confidences = [
        *metric_confidences.get_look_confidences(depth, look_enrichment_key),
        *metric_confidences.get_proxima_2020_light_confidences(depth),
    ]
    return create_metric(
        name=name,
        mode=mode,
        prefix=prefix,
        metric_class=metric_class,
        metric_module=metric_module,
        requirements=requirements,
        confidences=confidences,
        comment=comment,
        depth=depth,
        description=description,
        greater_better=greater_better,
        metric_name=metric_name,
        add_right_column=add_right_column,
        additional_kwargs=kwargs,
        wizard_filter=wizard_filter,
    )


def create_sinsig_metric(
        name,
        mode,
        wizard_filter: Optional[WizardFilter] = None,
        depth=30,
        prefix="sinsig-for-uno",
        metric_class="SinSigForUno",
        metric_module="sinsig_for_uno",
        comment=(
                "\nsinsig-kc-no-turbo-dcg-5 + right column"
                "\nsee: https://metrics.yandex-team.ru/admin/all-metrics/sinsig-kc-no-turbo-dcg-5"
        ),
        description=None,
        metric_name=None,
        add_right_column=True,
):
    return create_metric(
        name=name,
        mode=mode,
        prefix=prefix,
        metric_class=metric_class,
        metric_module=metric_module,
        requirements={
            "COMPONENT.judgements.sinsig_kc_no_turbo_judgement_values",
            "COMPONENT.judgements.sinsig_kc_no_turbo_slider_values",
        },
        confidences=metric_confidences.get_sinsig_confidences(depth),
        comment=comment,
        depth=depth,
        description=description,
        metric_name=metric_name,
        add_right_column=add_right_column,
        wizard_filter=wizard_filter,
    )


def create_proxima_metric(
        name,
        mode,
        wizard_filter: Optional[WizardFilter] = None,
        depth=30,
        prefix="proxima-v11-light-for-uno",
        metric_class="ProximaV11LightForUno",
        metric_module="sinsig_for_uno",
        comment="\nproxima-v11-light with right column",
        description=None,
        metric_name=None,
        add_right_column=True,
):
    return create_metric(
        name=name,
        mode=mode,
        prefix=prefix,
        metric_class=metric_class,
        metric_module=metric_module,
        requirements={
            "COMPONENT.judgements.comb_host_signal_enrichment_2021",
            "COMPONENT.judgements.dup_fine_2021",
            "COMPONENT.judgements.plag_fine_2021",
            "COMPONENT.judgements.sinsig_kc_no_turbo_judgement_values",
            "COMPONENT.judgements.sinsig_kc_no_turbo_slider_values",
            "COMPONENT.judgements.super_dup_only_fine_2021",
            "COMPONENT.judgements.super_text_dup_fine_2021",
            "SERP.query_param.cluster",
            "SERP.query_param.need_certain_host",
        },
        confidences=metric_confidences.get_proxima_2020_light_confidences(depth),
        comment=comment,
        depth=depth,
        description=description,
        metric_name=metric_name,
        add_right_column=add_right_column,
        wizard_filter=wizard_filter,
    )


def create_judged_signal_for_uno_metric(
        name,
        judgement,
        with_url: bool = True,
        depth: int = 5
):
    return MetricDescription(
        name=name,
        owner="robot-uno",
        revision=REVISION,
        url="svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia/quality/mstand_metrics/offline_production/metrics",
        metric_module="uno",
        metric_class="JudgedSignalForUno",
        kwargs={
            "depth": depth,
            "judgement": judgement,
            "with_url": with_url,
        },
        requirements=[
            f"COMPONENT.judgements.{judgement}",
        ],
        use_py3=True,
        responsible_users=RESPONSIBLE_USERS,
    )


def generate_uno_helper_metrics(prefix: str = "uno-v3", look_enrichment_key: str = "look"):
    uno_metric = partial(
        create_uno_metric,
        prefix=prefix,
        look_enrichment_key=look_enrichment_key,
    )

    # https://st.yandex-team.ru/OFFLINE-1642
    metrics = []

    # judged metrics
    for depth in [5, 30]:
        metrics.append(create_judged_signal_for_uno_metric(
            name="judged-sinsig-kc-no-turbo-for-uno-{}".format(depth),
            judgement="sinsig_kc_no_turbo_judgement_values",
            depth=depth,
        ))
        metrics.append(create_judged_signal_for_uno_metric(
            name="judged-comb-host-2021-for-uno-{}".format(depth),
            judgement="comb_host_signal_enrichment_2021",
            depth=depth,
        ))
        metrics.append(create_judged_signal_for_uno_metric(
            name="judged-plag-fine-2021-for-uno-{}".format(depth),
            judgement="plag_fine_2021",
            depth=depth,
        ))
        metrics.append(create_judged_signal_for_uno_metric(
            name="judged-dup-fine-2021-for-uno-{}".format(depth),
            judgement="dup_fine_2021",
            depth=depth,
        ))
        metrics.append(create_judged_signal_for_uno_metric(
            name="judged-super-dup-only-fine-2021-for-uno-{}".format(depth),
            judgement="super_dup_only_fine_2021",
            depth=depth,
        ))
        metrics.append(create_judged_signal_for_uno_metric(
            name="judged-super-text-dup-fine-2021-for-uno-{}".format(depth),
            judgement="super_text_dup_fine_2021",
            depth=depth,
        ))
        metrics.append(create_judged_signal_for_uno_metric(
            name=f"judged-{look_enrichment_key}-eval-for-uno-{depth}",
            judgement=f"{look_enrichment_key}_scores",
            depth=depth,
            with_url=False,
        ))

    # uno debug components
    metrics.append(MetricDescription(
        name=f"{prefix}-look-weight-avg",
        owner="robot-uno",
        revision=REVISION,
        url="svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia/quality/mstand_metrics/offline_production/metrics",
        metric_module="uno",
        metric_class="UnoV3LookWeight",
        kwargs={
            "depth": 30,
            "max_depth": 30,
            "look_scale": f"{look_enrichment_key}_scores",
            "look_height_scale": f"{look_enrichment_key}_screenshot_height",
        },
        requirements=[
            f"COMPONENT.judgements.{look_enrichment_key}_scores",
            f"COMPONENT.judgements.{look_enrichment_key}_screenshot_height",
            f"COMPONENT.judgements.element_screen_url"
        ],
        confidences=metric_confidences.get_look_confidences(30, look_enrichment_key),
        use_py3=True,
        description=(
            f"avg look weight in {prefix}"
        ),
        responsible_users=RESPONSIBLE_USERS,
    ))
    metrics.append(uno_metric(
        "5", "avg", depth=30, uno_coeff=0.0, swap_coeff=0.0,
        metric_name=f"{prefix}-height-penalty-avg", description="avg uno-v2 penalty for height"
    ))
    metrics.append(uno_metric(
        "5", "avg", depth=30, uno_coeff=0.0, height_coeff_left_column=0.0, height_coeff_right_column=0.0,
        metric_name=f"{prefix}-swap-penalty-avg", description="avg uno-v2 penalty for high look and low proxima"
    ))
    return metrics


def generate_uno_metrics(prefix: str = "uno-v3", look_enrichment_key: str = "look"):
    uno_metric = partial(
        create_uno_metric,
        prefix=prefix,
        look_enrichment_key=look_enrichment_key,
    )
    uno_impact_metric = partial(
        create_uno_metric,
        metric_class="UnoV3Impact",
        depth=4,
        prefix=f"{prefix}-impact",
        look_enrichment_key=look_enrichment_key,
    )

    # https://st.yandex-team.ru/OFFLINE-1642
    metrics = []

    # uno metrics
    metrics.append(uno_metric("5", "dcg", depth=5, metric_name=prefix))
    metrics.append(uno_metric("30", "dcg", depth=30))
    metrics.append(uno_metric("5", "sum", depth=5))
    metrics.append(uno_metric("", "dcg", depth=-1, metric_name=f"{prefix}-right-column-only"))
    metrics.append(uno_metric("", "dcg", depth=5, add_right_column=False, metric_name=f"{prefix}-left-column-only"))
    metrics.append(uno_metric(
        "5", "dcg", depth=5, use_const_weights=True,
        description=(
            f"При различных прокрасках с {prefix} (в том числе, одно красится, второе нет) есть риск, "
            "что эксперимент вышел за границы применимости метрики, обратитесь к ответственным "
            "https://st.yandex-team.ru/OFFLINE-1663"
        ),
        metric_name=f"{prefix}-const",
    ))

    # wizard metrics
    for name, wizard_filters in WIZARD_FILTERS.items():
        if isinstance(wizard_filters, dict):
            wizard_filter = WizardFilter.merge_filters(list(wizard_filters.values()))
            for sub_name, sub_wizard_filter in wizard_filters.items():
                if sub_name != OTHER:
                    metrics.append(uno_metric(sub_name, "avg", sub_wizard_filter))
                    metrics.append(uno_impact_metric(f"4-{sub_name}", "dcg", sub_wizard_filter))
        else:
            wizard_filter = wizard_filters
        metrics.append(uno_metric(name, "avg", wizard_filter))
        metrics.append(uno_impact_metric(f"4-{name}", "dcg", wizard_filter))

    # plugins metrics
    metrics.append(uno_metric(
        "snip-all",
        "avg",
        WizardFilter(plugins=all_snipet_plugins, plugins_has_data=False),
        description="sum uno-v2 avg for specsnippets\nhttps://st.yandex-team.ru/BLNDR-5236",
        greater_better=False,
    ))
    metrics.append(uno_metric(
        "snip-has_data-all",
        "avg",
        WizardFilter(plugins=all_snipet_plugins, plugins_has_data=True),
        description="sum uno-v2 avg for specsnippets\nhttps://st.yandex-team.ru/BLNDR-5236",
        greater_better=False,
    ))
    for name, plugins_list in blender_slices.blender_snippets.items():
        metrics.append(uno_metric("snip-{}".format(name), "avg", WizardFilter(plugins=set(plugins_list), plugins_has_data=False)))
        metrics.append(uno_metric("snip-has_data-{}".format(name), "avg", WizardFilter(plugins=set(plugins_list), plugins_has_data=True)))

    return metrics


def generate_look_metrics(prefix: str = "look", look_enrichment_key: str = "look"):
    look_metric = partial(
        create_look_metric,
        prefix=prefix,
        look_enrichment_key=look_enrichment_key,
    )
    defect_rate = partial(
        look_metric,
        additional_kwargs={
            "defect_rate_threshold": 0
        }
    )
    coverage_prefix = "uno"

    # https://st.yandex-team.ru/OFFLINE-980
    metrics = []

    # look metrics
    metrics.append(look_metric("5", "dcg", depth=5))
    metrics.append(look_metric("30", "dcg", depth=30))
    metrics.append(look_metric("5", "sum", depth=5))
    metrics.append(look_metric("", "dcg", depth=-1, metric_name=f"{prefix}-right-column-only"))
    metrics.append(look_metric("", "dcg", depth=5, add_right_column=False, metric_name=f"{prefix}-left-column-only"))

    # wizard metrics
    for name, wizard_filters in WIZARD_FILTERS.items():
        if isinstance(wizard_filters, dict):
            wizard_filter = WizardFilter.merge_filters(list(wizard_filters.values()))
            for sub_name, sub_wizard_filter in wizard_filters.items():
                if sub_name != OTHER:
                    metrics.append(look_metric(sub_name, "avg", sub_wizard_filter))
                    metrics.append(look_metric(sub_name, "coverage", sub_wizard_filter, prefix=coverage_prefix))
                    # for depth in {1, 2, 5, 30}:
                    for depth in {5}:
                        metrics.append(defect_rate(f"{depth}-{sub_name}", "defect-rate", sub_wizard_filter, depth=depth))
                        metrics.append(defect_rate(f"judged-{depth}-{sub_name}", "defect-rate", sub_wizard_filter, depth=depth))
                        metrics.append(defect_rate(f"{depth}-{sub_name}", "defect-rate-dcg", sub_wizard_filter, depth=depth))
        else:
            wizard_filter = wizard_filters
        metrics.append(look_metric(name, "avg", wizard_filter))
        metrics.append(look_metric(name, "coverage", wizard_filter, prefix=coverage_prefix))
        for depth in {1, 2, 5, 30}:
            metrics.append(defect_rate(f"{depth}-{name}", "defect-rate", wizard_filter, depth=depth))
            metrics.append(defect_rate(f"judged-{depth}-{name}", "defect-rate", wizard_filter, depth=depth))
            metrics.append(defect_rate(f"{depth}-{name}", "defect-rate-dcg", wizard_filter, depth=depth))

    # plugins metrics
    metrics.append(look_metric(
        "snip-all",
        "avg",
        WizardFilter(plugins=all_snipet_plugins, plugins_has_data=False),
        description=f"sum {prefix} avg for specsnippets\nhttps://st.yandex-team.ru/BLNDR-5236",
        greater_better=False,
    ))
    metrics.append(look_metric(
        "snip-has_data-all",
        "avg",
        WizardFilter(plugins=all_snipet_plugins, plugins_has_data=True),
        description=f"sum {prefix} avg for specsnippets\nhttps://st.yandex-team.ru/BLNDR-5236",
        greater_better=False,
    ))
    for name, plugins_list in blender_slices.blender_snippets.items():
        metrics.append(look_metric("snip-{}".format(name), "avg", WizardFilter(plugins=set(plugins_list), plugins_has_data=False)))
        metrics.append(look_metric("snip-has_data-{}".format(name), "avg", WizardFilter(plugins=set(plugins_list), plugins_has_data=True)))
    return metrics


def generate_look_h_metrics(prefix: str = "look-height-v3", look_enrichment_key: str = "look"):
    look_height_metric = partial(
        create_look_height_metric,
        prefix=prefix,
        look_enrichment_key=look_enrichment_key,
    )
    look_height_impact_metric = partial(
        create_look_height_metric,
        prefix=f"{prefix}-impact", metric_class="UnoV3Impact",
        depth=4,
        look_enrichment_key=look_enrichment_key,
    )
    defect_rate = partial(
        look_height_metric,
        additional_kwargs={
            "defect_rate_threshold": -0.5
        }
    )
    impact_defect_rate = partial(
        create_look_height_metric,
        prefix=f"{prefix}-impact-defect-rate", metric_class="UnoImpactDefectRate",
        metric_module="uno_defect_rate",
        look_enrichment_key=look_enrichment_key,
        additional_kwargs={
            "impact_defect_rate_th": -1.0,
            "look_defect_rate_th": 2.0,
        }
    )
    # https://st.yandex-team.ru/OFFLINE-980
    metrics = []
    # look-height metrics
    metrics.append(look_height_metric("5", "dcg", depth=5))
    metrics.append(look_height_metric("30", "dcg", depth=30))
    metrics.append(look_height_metric("5", "sum", depth=5))
    metrics.append(look_height_metric("", "dcg", depth=-1, metric_name=f"{prefix}-right-column-only"))
    metrics.append(look_height_metric("", "dcg", depth=5, add_right_column=False, metric_name=f"{prefix}-left-column-only"))

    # wizard metrics
    for name, wizard_filters in WIZARD_FILTERS.items():
        if isinstance(wizard_filters, dict):
            wizard_filter = WizardFilter.merge_filters(list(wizard_filters.values()))
            for sub_name, sub_wizard_filter in wizard_filters.items():
                if sub_name != OTHER:
                    metrics.append(look_height_metric(sub_name, "avg", sub_wizard_filter))
                    metrics.append(look_height_impact_metric(f"4-{sub_name}", "dcg", sub_wizard_filter))
                    # for depth in {1, 2, 5, 30}:
                    for depth in {5}:
                        metrics.append(defect_rate(f"{depth}-{sub_name}", "defect-rate", sub_wizard_filter, depth=depth))
                        metrics.append(defect_rate(f"judged-{depth}-{sub_name}", "defect-rate", sub_wizard_filter, depth=depth))
                        metrics.append(defect_rate(f"{depth}-{sub_name}", "defect-rate-dcg", sub_wizard_filter, depth=depth))
                    metrics.append(impact_defect_rate(f"{5}-{sub_name}", "dcg", sub_wizard_filter, depth=5))
                    # metrics.append(impact_defect_rate(f"{30}-{sub_name}", "dcg", sub_wizard_filter, depth=30))
        else:
            wizard_filter = wizard_filters
        metrics.append(look_height_metric(name, "avg", wizard_filter))
        metrics.append(look_height_impact_metric(f"4-{name}", "dcg", wizard_filter))
        for depth in {1, 2, 5, 30}:
            metrics.append(defect_rate(f"{depth}-{name}", "defect-rate", wizard_filter, depth=depth))
            metrics.append(defect_rate(f"judged-{depth}-{name}", "defect-rate", wizard_filter, depth=depth))
            metrics.append(defect_rate(f"{depth}-{name}", "defect-rate-dcg", wizard_filter, depth=depth))
        metrics.append(impact_defect_rate(f"{5}-{name}", "dcg", wizard_filter, depth=5))
        metrics.append(impact_defect_rate(f"{30}-{name}", "dcg", wizard_filter, depth=30))

    # plugins metrics
    metrics.append(look_height_metric(
        "snip-all",
        "avg",
        WizardFilter(plugins=all_snipet_plugins, plugins_has_data=False),
        description="sum look-height avg for specsnippets\nhttps://st.yandex-team.ru/BLNDR-5236",
        greater_better=False,
    ))
    metrics.append(look_height_metric(
        "snip-has_data-all",
        "avg",
        WizardFilter(plugins=all_snipet_plugins, plugins_has_data=True),
        description="sum look-height avg for specsnippets\nhttps://st.yandex-team.ru/BLNDR-5236",
        greater_better=False,
    ))
    for name, plugins_list in blender_slices.blender_snippets.items():
        metrics.append(look_height_metric("snip-{}".format(name), "avg", WizardFilter(plugins=set(plugins_list), plugins_has_data=False)))
        metrics.append(look_height_metric("snip-has_data-{}".format(name), "avg", WizardFilter(plugins=set(plugins_list), plugins_has_data=True)))
    return metrics


main_wizard_filters = {
    "ALL_ORGANIC",
    "ALL_WIZARDS",
    "IMAGE",
    "ENTITY_SEARCH",
    "VIDEO",
    "ALL_FACTS",
    "GEO",
}


def create_relevance_metrics(metric_name: str, metric_generator: callable, impact_metric_generator: callable):
    metrics = []
    # uno metrics
    metrics.append(metric_generator("5", "dcg", depth=5, metric_name=metric_name))
    metrics.append(metric_generator("", "dcg", depth=-1, metric_name=f"{metric_name}-right-column-only"))
    metrics.append(metric_generator("", "dcg", depth=5, add_right_column=False, metric_name=f"{metric_name}-left-column-only"))

    # wizard metrics
    for name, wizard_filters in WIZARD_FILTERS.items():
        if name in main_wizard_filters:
            if isinstance(wizard_filters, dict):
                wizard_filter = WizardFilter.merge_filters(list(wizard_filters.values()))
            else:
                wizard_filter = wizard_filters
            metrics.append(metric_generator(name, "avg", wizard_filter))
            metrics.append(impact_metric_generator(f"4-{name}", "dcg", wizard_filter))
    return metrics


def create_sinsig_metrics():
    metric_name = "sinsig-for-uno"
    return create_relevance_metrics(
        metric_name=metric_name,
        metric_generator=create_sinsig_metric,
        impact_metric_generator=partial(
            create_sinsig_metric,
            metric_class="SinSigForUnoImpact",
            depth=4,
            prefix=f"{metric_name}-impact",
        ),
    )


def create_proxima_metrics():
    metric_name = "proxima-v11-light-for-uno"
    return create_relevance_metrics(
        metric_name=metric_name,
        metric_generator=create_proxima_metric,
        impact_metric_generator=partial(
            create_proxima_metric,
            metric_class="ProximaV11LightForUnoImpact",
            depth=4,
            prefix=f"{metric_name}-impact",
        ),
    )


def create_look_by_th_metrics(prefix="look-v2-by-threshold", look_enrichment_key="look_v2"):
    metrics = []
    look_metric = partial(
        create_look_metric,
        prefix=prefix,
        look_enrichment_key=look_enrichment_key,
        metric_class="LookByThreshold",
        metric_module="look_by_threshold",
    )
    metrics.append(look_metric(None, "avg", WIZARD_FILTERS["RELATED_DISCOVERY"], additional_kwargs={"threshold": 0.0}, metric_name=f"{prefix}-0_8-avg-RELATED_DISCOVERY"))
    metrics.append(look_metric(None, "avg", WIZARD_FILTERS["RELATED_DISCOVERY"], additional_kwargs={"threshold": 0.5}, metric_name=f"{prefix}-0_5-avg-RELATED_DISCOVERY"))
    metrics.append(look_metric(None, "avg", WIZARD_FILTERS["RELATED_DISCOVERY"], additional_kwargs={"threshold": 0.8}, metric_name=f"{prefix}-0-avg-RELATED_DISCOVERY"))

    metrics.append(look_metric(None, "avg", WIZARD_FILTERS["TRANSLATE"], additional_kwargs={"threshold": 0.8}, metric_name=f"{prefix}-0_8-avg-TRANSLATE"))

    metrics.append(look_metric(None, "avg", WIZARD_FILTERS["WEATHER"], additional_kwargs={"threshold": 0.8}, metric_name=f"{prefix}-0_8-avg-WEATHER"))
    return metrics


def main():
    cli_args = mc_common.parse_args(default_dir="uno")
    umisc.configure_logger()

    logging.info("generating UNO metrics")
    metrics = [
        *generate_uno_metrics(prefix="uno-v3", look_enrichment_key="look_v3"),
        *generate_uno_helper_metrics(prefix="uno-v3", look_enrichment_key="look_v3"),
        *generate_look_metrics(prefix="look-v3", look_enrichment_key="look_v3"),
        *generate_look_h_metrics(prefix="look-height-v3", look_enrichment_key="look_v3"),
        *create_sinsig_metrics(),
        *create_proxima_metrics(),
        *create_look_by_th_metrics(prefix="look-v3-by-threshold", look_enrichment_key="look_v3"),
    ]
    mc_common.save_generated(metrics, cli_args)


if __name__ == "__main__":
    import os

    os.chdir("metrics_tools")
    main()
