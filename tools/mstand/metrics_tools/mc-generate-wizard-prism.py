#!/usr/bin/env python3

import proxima_parts_2020
import proxima_parts_wizard
from omglib import mc_common
from proxima_description import ProximaDescription, ProximaPart

import yaqutils.misc_helpers as umisc

REVISION_2020 = 7848902
WIKI = "https://wiki.yandex-team.ru/JandeksPoisk/KachestvoPoiska/AnalyticsAndProductization/offline-metrics/" \
       "Current-metrics-and-baskets/"

prism_weight_mult = " * (float(serp_data.get('query_param.prism_weight', 0.89)) / 0.89) "
prism_weight_requirements = [
    "SERP.query_param.prism_weight"
]

prism_irrel_aggregate_script = (
    "(float(sum(all_custom_formulas['is_irrel_target_wizard'])) / sum(all_custom_formulas['is_target_wizard']))" + prism_weight_mult +
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
    label_script_parts=["D.custom_formulas['is_irrel_target_wizard']"],
    custom_formulas={"is_irrel": "int('IRREL' in L_raw.sinsig_kc_no_turbo_judgement_values)"},
    raw_signals=["sinsig_kc_no_turbo_judgement_values"],
    custom_formulas_ap={
        "is_irrel_target_wizard": "D.custom_formulas['is_irrel'] * D.custom_formulas['is_target_wizard']",
    },
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


def prism_wizard_impact(part, part_impact):
    part.merge_with(part_impact)
    assert not part.aggregate_script
    wizard_impact_aggregate_script = (
        "sum(value/(pos+1) for pos, value in enumerate(all_custom_formulas['value']) if pos<5)"
        " - sum(value/(pos+1) for pos, value in enumerate(value for value, is_target in"
        " zip(all_custom_formulas['value'],all_custom_formulas['is_target_wizard']) if not is_target) if pos<5)"
    )
    part.aggregate_script = (
        "(" + wizard_impact_aggregate_script + ")" +
        prism_weight_mult
    )
    part.label_script_parts = ["D.custom_formulas['value']"]
    part.custom_formulas_ap["value"] = (
        "0.69 * D.custom_formulas['sinsig'] "
        "+ 0.15/0.882 * D.custom_formulas['comb'] * D.custom_formulas['cluster_boost']"
    )


def generate_blender_metrics():
    metrics = []

    proxima_power_prism = ProximaPart(
        custom_formulas={
            "is_vital": "{} >= 1.5".format(proxima_parts_2020.SINSIG),
        },
        aggregate_script=(
            proxima_parts_2020.POWER_07 +
            prism_weight_mult
        ),
        requirements=prism_weight_requirements,
    )
    proxima2020_wizard_prism = ProximaDescription(
        name="proxima2020-wizard-prism",
        parts=(
            proxima_parts_2020.proxima_sinsig_light,
            proxima_parts_2020.proxima_comb,
            proxima_parts_2020.div_position,
            proxima_power_prism,
        ),
        description=u"proxima без dbd для колдунщиков взвешенная на prism_weight",
        revision=REVISION_2020,
        wiki=WIKI,
    )
    proxima_parts_2020.wizard_dup_plag_conf(proxima2020_wizard_prism.part)
    metrics.append(proxima2020_wizard_prism)

    for name, impact_part in proxima_parts_wizard.wiz_slices_parts.items():
        description = u"proxima2020-wizard-prism с выкидыванием колдунщика {} \n" \
                      u"(https://st.yandex-team.ru/OFFLINESUP-443)".format(name)
        impact_2020 = ProximaDescription(
            name="proxima2020-{}-slices-impact-prism".format(name),
            parts=(
                proxima_parts_2020.sinsig,
                proxima_parts_2020.comb_with_sinsig_and_cluster,
                proxima_parts_wizard.slices_part,
                proxima_parts_2020.div_position,
            ),
            requirements=prism_weight_requirements,
            description=description,
            revision=REVISION_2020,
            depth=10,
        )
        prism_wizard_impact(impact_2020.part, impact_part)
        metrics.append(impact_2020)

    for name, impact_part in proxima_parts_wizard.wiz_slices_parts.items():
        description = u"sinsig-wizard-15-irrel-rate-{}-min1-prism \n" \
                      u"(https://st.yandex-team.ru/BLNDR-4121)".format(name)
        metrics.append(
            ProximaDescription(
                name="sinsig-wizard-15-irrel-rate-{}-min1-prism".format(name),
                parts=(
                    irrel_base,
                    impact_part
                ),
                requirements=prism_weight_requirements,
                description=description,
                revision=REVISION_2020,
                depth=15,
                max_depth=15,
                greater_better=False,
                aggregate_script=prism_irrel_aggregate_script,
            )
        )
    return metrics


def main():
    cli_args = mc_common.parse_args(default_dir="proxima-wizard-prism")
    umisc.configure_logger()

    metrics = generate_blender_metrics()
    mc_common.save_generated(metrics, cli_args)


if __name__ == "__main__":
    main()
