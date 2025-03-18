#!/usr/bin/env python3
# coding=utf-8

import proxima_parts_2020
import proxima_parts_wizard
from omglib import mc_common
from proxima_description import ProximaDescription
from proxima_description import ProximaPart

import yaqutils.misc_helpers as umisc

REVISION_2020 = 7767749
REVISION_YANDEX_SERVICES = 7830850
WIKI = "https://wiki.yandex-team.ru/JandeksPoisk/KachestvoPoiska/AnalyticsAndProductization/offline-metrics/" \
       "Current-metrics-and-baskets/"

irrel_aggregate_script = (
    "(float(sum(all_custom_formulas['is_irrel_target_wizard'])) / sum(all_custom_formulas['is_target_wizard'])) "
    "if (sum(all_custom_formulas['is_target_wizard']) > 0) "
    "else None"
)

irrel_base = ProximaPart(
    confidences=[
        {
            "name": "judged-sinsig-kc-no-turbo-judgement-values-15",
            "threshold": 0.5,
            "condition": "GREATER",
            "requirements": [],
            "regional": None,
            "yandex": False,
        },
        {
            "name": "serp-failed",
            "threshold": 0.003,
            "condition": "SMALLER",
            "requirements": [],
            "regional": None,
            "yandex": False,
        },
    ],
    requirements=[
        "COMPONENT.json.slices",
        "COMPONENT.judgements.sinsig_kc_no_turbo_judgement_values",
    ],
    custom_formulas={"is_irrel": "int('IRREL' in L_raw.sinsig_kc_no_turbo_judgement_values)"},
    raw_signals=["sinsig_kc_no_turbo_judgement_values"],
)
irrel_base_label_script = ProximaPart(
    label_script_parts=["D.custom_formulas['is_irrel_target_wizard']"],
    custom_formulas_ap={
        "is_irrel_target_wizard": "D.custom_formulas['is_irrel'] * D.custom_formulas['is_target_wizard']",
    },
)
irrel_snippet_label_script = ProximaPart(
    label_script_parts=["D.custom_formulas['is_irrel'] * D.custom_formulas['is_target_wizard']"]
)

slices_coverage = ProximaPart(
    confidences=[
        {
            "name": "serp-failed",
            "threshold": 0.001,
            "condition": "SMALLER",
            "requirements": [],
            "regional": None,
            "yandex": False,
        },
    ],
    requirements=[
        "COMPONENT.json.slices",
    ],
    label_script_parts=["D.custom_formulas['is_target_wizard']"],
)
snippet_coverage = ProximaPart(
    confidences=[
        {
            "name": "serp-failed",
            "threshold": 0.001,
            "condition": "SMALLER",
            "requirements": [],
            "regional": None,
            "yandex": False,
        },
    ],
    requirements=[
        "COMPONENT.json.SearchRuntimeInfo",
    ],
    label_script_parts=["D.custom_formulas['is_target_wizard']"],
)


def generate_blender_metrics():
    metrics = []

    proxima2020_wizard = ProximaDescription(
        name="proxima2020-wizard",
        parts=(
            proxima_parts_2020.proxima_sinsig_light,
            proxima_parts_2020.proxima_comb,
            proxima_parts_2020.proxima_power,
            proxima_parts_2020.div_position,
        ),
        description=u"proxima без dbd для колдунщиков",
        revision=REVISION_2020,
        wiki=WIKI,
        use_py3=True,
    )
    proxima_parts_2020.wizard_dup_plag_conf(proxima2020_wizard.part)
    metrics.append(proxima2020_wizard)

    for name, impact_part in proxima_parts_wizard.wiz_parts.items():
        description = u"proxima2020-wizard с выкидыванием колдунщика {} \n" \
                      u"(https://st.yandex-team.ru/OFFLINESUP-443)".format(name)
        impact_2020 = ProximaDescription(
            name="proxima2020-{}-impact".format(name),
            parts=(
                proxima_parts_2020.sinsig,
                proxima_parts_2020.comb_with_sinsig_and_cluster,
                proxima_parts_2020.div_position,
            ),
            description=description,
            revision=REVISION_2020,
            depth=10,
            use_py3=True,
        )
        proxima_parts_2020.wizard_impact(impact_2020.part, impact_part)
        metrics.append(impact_2020)

    for name, impact_part in proxima_parts_wizard.wiz_slices_parts.items():
        description = u"proxima2020-wizard с выкидыванием колдунщика {} \n" \
                      u"(https://st.yandex-team.ru/OFFLINESUP-443)".format(name)
        impact_2020 = ProximaDescription(
            name="proxima2020-{}-slices-impact".format(name),
            parts=(
                proxima_parts_2020.sinsig,
                proxima_parts_2020.comb_with_sinsig_and_cluster,
                proxima_parts_wizard.slices_part,
                proxima_parts_2020.div_position,
            ),
            description=description,
            revision=REVISION_2020,
            depth=10,
            use_py3=True,
        )
        proxima_parts_2020.wizard_impact(impact_2020.part, impact_part)
        metrics.append(impact_2020)

    for name, impact_part in proxima_parts_wizard.wiz_services_parts.items():
        description = u"proxima2020-wizard с выкидыванием сервиса {} \n" \
                      u"(https://st.yandex-team.ru/BLNDR-5168)".format(name)
        impact_2020 = ProximaDescription(
            name="proxima2020-service-{}-impact".format(name),
            parts=(
                proxima_parts_2020.sinsig,
                proxima_parts_2020.comb_with_sinsig_and_cluster,
                proxima_parts_wizard.services_part,
                proxima_parts_2020.div_position,
            ),
            description=description,
            revision=REVISION_YANDEX_SERVICES,
            depth=10,
            use_py3=True,
        )
        proxima_parts_2020.wizard_impact(impact_2020.part, impact_part)
        metrics.append(impact_2020)

    for name, impact_part in proxima_parts_wizard.wiz_slices_parts.items():
        description = u"sinsig-wizard-15-irrel-rate-{}-min1 \n" \
                      u"(https://st.yandex-team.ru/BLNDR-4121)".format(name)
        metrics.append(
            ProximaDescription(
                name="sinsig-wizard-15-irrel-rate-{}-min1".format(name),
                parts=(
                    irrel_base,
                    irrel_base_label_script,
                    impact_part
                ),
                description=description,
                revision=REVISION_2020,
                depth=15,
                max_depth=15,
                greater_better=False,
                aggregate_script=irrel_aggregate_script,
                use_py3=True,
            )
        )

        metrics.append(
            ProximaDescription(
                name="sinsig-wizard-15-irrels-{}-min1".format(name),
                parts=(
                    irrel_base,
                    irrel_base_label_script,
                    impact_part
                ),
                description=description,
                revision=REVISION_2020,
                depth=15,
                max_depth=15,
                greater_better=False,
                use_py3=True,
            )
        )

        metrics.append(
            ProximaDescription(
                name="slices-coverage-{}".format(name),
                parts=(
                    slices_coverage,
                    impact_part
                ),
                description="{} converage https://st.yandex-team.ru/BLNDR-3884".format(name),
                revision=REVISION_2020,
                depth=30,
                max_depth=30,
                greater_better=False,
                use_py3=True,
            )
        )

    for name, snippet_part in proxima_parts_wizard.wiz_snippets_parts.items():
        coverage_description = u"coverage for snippet {} by Plugins\n" \
                               u"(https://st.yandex-team.ru/BLNDR-5504)".format(name)
        metrics.append(
            ProximaDescription(
                name="coverage-snip-{}".format(name),
                parts=(
                    snippet_coverage,
                    snippet_part,
                ),
                description=coverage_description,
                revision=REVISION_YANDEX_SERVICES,
                depth=30,
                max_depth=30,
                use_py3=True,
            )
        )

        metrics.append(
            ProximaDescription(
                name="sinsig-snippet-irrels-{}".format(name),
                parts=(
                    snippet_part,
                    irrel_base,
                    irrel_snippet_label_script,
                ),
                description=u"irrels count for snippet {} by Plugins\n" \
                            u"https://st.yandex-team.ru/BLNDR-5236".format(name),
                revision=REVISION_YANDEX_SERVICES,
                depth=30,
                max_depth=30,
                use_py3=True,
            )
        )

    for name, snippet_part in proxima_parts_wizard.wiz_snippets_parts_has_data.items():
        coverage_description = u"coverage for snippet {} by Plugins, DisabledP, FilteredP\n" \
                               u"(https://st.yandex-team.ru/BLNDR-5504)".format(name)
        metrics.append(
            ProximaDescription(
                name="coverage-snip-has_data-{}".format(name),
                parts=(
                    snippet_coverage,
                    snippet_part,
                ),
                description=coverage_description,
                revision=REVISION_YANDEX_SERVICES,
                depth=30,
                max_depth=30,
                use_py3=True,
            )
        )

        metrics.append(
            ProximaDescription(
                name="sinsig-snippet-irrels-has_data-{}".format(name),
                parts=(
                    snippet_part,
                    irrel_base,
                    irrel_snippet_label_script,
                ),
                description=u"irrels count for snippet {} by Plugins, DisabledP, FilteredP\n" \
                            u"https://st.yandex-team.ru/BLNDR-5236".format(name),
                revision=REVISION_YANDEX_SERVICES,
                depth=30,
                max_depth=30,
                use_py3=True,
            )
        )

    return metrics


def main():
    cli_args = mc_common.parse_args(default_dir="proxima-wizard")
    umisc.configure_logger()

    metrics = generate_blender_metrics()
    mc_common.save_generated(metrics, cli_args)


if __name__ == "__main__":
    main()
