#!/usr/bin/env python3
# coding=utf-8

import logging
from typing import List

from omglib import MetricDescription, mc_common

import yaqutils.misc_helpers as umisc

REVISION = 8426216

wizard_types = {
    "afisha",
    "afisha-generic-rubric",
    "alice",
    "article",
    "auto_2",
    "autoparts/button",
    "autoparts/common",
    "autoparts/thumbs-text",
    "autoru/thumbs-price",
    "autoru/thumbs-text",
    "bno",
    "browser",
    "buy_tickets",
    "calculator",
    "calories_fact",
    "colors",
    "companies",
    "counterparty",
    "dict_fact",
    "distance_fact",
    "distr",
    "drugs",
    "eda",
    "election",
    "entity-fact",
    "entity/afisha",
    "entity/movie",
    "entity/reviews",
    "entity_search",
    "extended_snippet",
    "fact_instruction",
    "fastres",
    "games_category",
    "games_one",
    "general/gallery",
    "general/text",
    "geo_common_wizard",
    "health",
    "health_encyclopedia",
    "image_fact",
    "images",
    "informer",
    "lyrics",
    "maps",
    "market_constr",
    "market_reask_gallery",
    "math",
    "multiple_facts",
    "musicplayer",
    "my_ip",
    "news",
    "panoramas",
    "poetry_lover",
    "post_indexes",
    "pseudo",
    "pseudo_fast",
    "q",
    "rabota",
    "random_number",
    "rasp",
    "realty/add",
    "realty/complex",
    "realty/fold",
    "realty/gallery",
    "realty/site-gallery",
    "realty/text",
    "realty/thumb",
    "related_discovery",
    "rich_fact",
    "route",
    "single-row-carousel",
    "special/event",
    "sport/livescore",
    "stocks",
    "suggest-subsearch",
    "suggest_fact",
    "table_fact",
    "taxi",
    "taxi_light",
    "time",
    "traffic",
    "translate",
    "units_converter",
    "video",
    "videowiz",
    "weather",
    "weather_month",
    "yabs_proxy",
    "ydo",
    "zen",
    "znatoki_fact"
}


def generate_impact_metric(wizard_baobab_names: List[str], metric_name_suffix: str):
    kwargs = {
        'target_baobab_names': wizard_baobab_names,
        "judged": False,
        "depth": 5,
        "max_depth": 10
    }

    requirements = [
        "COMPONENT.judgements.comb_host_signal_enrichment_2021",
        "COMPONENT.judgements.dup_fine_2021",
        "COMPONENT.judgements.plag_fine_2021",
        "COMPONENT.judgements.sinsig_kc_no_turbo_judgement_values",
        "COMPONENT.judgements.sinsig_kc_no_turbo_slider_values",
        "COMPONENT.judgements.super_dup_only_fine_2021",
        "COMPONENT.judgements.super_text_dup_fine_2021",
        "COMPONENT.judgements.sens_url_quality",
        "COMPONENT.judgements.legal_film_url",
        "COMPONENT.judgements.ecom_biz_kernel_enrichment2",
        "COMPONENT.judgements.ecom_host_quality_enrichment",
        "COMPONENT.judgements.bad_reviews_enrichment",

        "COMPONENT.text.baobabWizardName",

        "SERP.query_param.cluster",
        "SERP.query_param.need_certain_host",
        "SERP.query_param.sens_category",
        "SERP.query_param.commercial_query",
        "SERP.query_param.video_piracy_predict"
    ]
    confidences = [
        {
            "name": "comb-host-signal-2021-judged-5",
            "threshold": 0.92,
            "condition": "GREATER",
            "requirements": [],
            "regional": None,
            "yandex": False
        },
        {
            "name": "judged-plag-fine-5",
            "threshold": 0.85,
            "condition": "GREATER",
            "requirements": [],
            "regional": None,
            "yandex": False
        },
        {
            "name": "judged-dup-fine-5",
            "threshold": 0.85,
            "condition": "GREATER",
            "requirements": [],
            "regional": None,
            "yandex": False
        },
        {
            "name": "empty-serp",
            "threshold": 0.01,
            "condition": "SMALLER",
            "requirements": [],
            "regional": None,
            "yandex": False
        },
        {
            "name": "empty-serp",
            "threshold": 0.001,
            "condition": "SMALLER",
            "requirements": [],
            "regional": None,
            "yandex": True
        },
        {
            "name": "judged-sinsig-kc-no-turbo-judgement-values-5",
            "threshold": 0.999,
            "condition": "GREATER",
            "requirements": [],
            "regional": None,
            "yandex": False
        },
        {
            "name": "serp-failed",
            "threshold": 0.003,
            "condition": "SMALLER",
            "requirements": [],
            "regional": None,
            "yandex": False
        },
        {
            "name": "judged-neg-reviews-host-signal-proxima-5",
            "threshold": 0.7,
            "condition": "GREATER",
            "requirements": [],
            "regional": None,
            "yandex": False
        },
        {
            "name": "judged-ecom-host-quality-proxima-5",
            "threshold": 0.3,
            "condition": "GREATER",
            "requirements": [],
            "regional": None,
            "yandex": False
        },
        {
            "name": "judged-ecom-kernel-ugc-proxima-5",
            "threshold": 0.9,
            "condition": "GREATER",
            "requirements": [],
            "regional": None,
            "yandex": True
        },
        {
            "name": "judged-ecom-kernel-ugc-proxima-5",
            "threshold": 0.8,
            "condition": "GREATER",
            "requirements": [],
            "regional": None,
            "yandex": False
        },
        {
            "name": "judged-legal-film-url-proxima-5",
            "threshold": 0.99,
            "condition": "GREATER",
            "requirements": [],
            "regional": None,
            "yandex": False
        },
        {
            "name": "judged-sens-url-quality-proxima-5",
            "threshold": 0.99,
            "condition": "GREATER",
            "requirements": [],
            "regional": None,
            "yandex": False
        },
        {
            "name": "judged-super-dup-only-fine-5",
            "threshold": 0.85,
            "condition": "GREATER",
            "requirements": [],
            "regional": None,
            "yandex": False
        },
        {
            "name": "judged-super-text-dup-fine-5",
            "threshold": 0.85,
            "condition": "GREATER",
            "requirements": [],
            "regional": None,
            "yandex": False
        }
    ]

    parsed_metric_name = metric_name_suffix.replace('/', '_')
    metric_name = f"proxima-v9-impact-baobab-{parsed_metric_name}"

    return MetricDescription(
        name=metric_name,
        owner="robot-proxima",
        description="",
        revision=REVISION,
        url="svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia/quality/mstand_metrics/offline_production/metrics",
        metric_module="proxima_v9_metrics",
        metric_class="ProximaV9Impact",
        kwargs=kwargs,
        requirements=requirements,
        confidences=confidences,
        use_py3=True,
        greater_better=True,
    )


def generate_impact_metrics():
    metrics = list()
    for baobab_type in wizard_types:
        metrics.append(generate_impact_metric([baobab_type], baobab_type))

    metrics.append(generate_impact_metric(list(wizard_types), 'all-wizards'))

    return metrics


def main():
    cli_args = mc_common.parse_args(default_dir="proxima")
    umisc.configure_logger()

    logging.info("generating proxima-v9 impact metrics")
    metrics = [
        *generate_impact_metrics(),
    ]
    mc_common.save_generated(metrics, cli_args)


if __name__ == "__main__":
    import os

    os.chdir("metrics_tools")
    main()
