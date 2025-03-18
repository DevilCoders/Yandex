#!/usr/bin/env python3

import clusters
import proxima_parts_2020 as parts_2020
from omglib import mc_common
from proxima_description import ProximaDescription

import yaqutils.misc_helpers as umisc

REVISION_2020 = 7767749
WIKI = "https://wiki.yandex-team.ru/JandeksPoisk/KachestvoPoiska/AnalyticsAndProductization/offline-metrics/" \
       "Current-metrics-and-baskets/"


def generate_proxima():
    metrics = list()

    for cluster_number, cluster in clusters.CLUSTERS.items():
        name = "CLUSTER-{:02d}-proxima2020".format(cluster_number)
        description = u"cluster {}: {}".format(cluster.name, cluster.description)
        cluster_proxima = ProximaDescription(
            name=name,
            parts=(
                parts_2020.proxima_rel,
                parts_2020.proxima_comb,
                parts_2020.proxima_power,
                parts_2020.div_position,
            ),
            description=description,
            revision=REVISION_2020,
            wiki=WIKI,
        )
        parts_2020.cluster(cluster_proxima.part, cluster_number)
        metrics.append(
            cluster_proxima
        )

    for c in range(1, 11):
        name = "proxima2020-minus-{:02d}".format(c)
        power_minus_c = parts_2020.proxima_power.copy()
        power_minus_c.aggregate_script += " - {:.1f}".format(c / 10.0)
        metrics.append(
            ProximaDescription(
                name=name,
                parts=(
                    parts_2020.proxima_rel,
                    parts_2020.proxima_comb,
                    power_minus_c,
                    parts_2020.div_position,
                ),
                description=u"proxima - основная офлайн метрика ранжирования (со степенью)",
                revision=REVISION_2020,
                wiki=WIKI,
            )
        )

    metrics.extend([
        ProximaDescription(
            name="proxima2020",
            parts=(
                parts_2020.proxima_rel,
                parts_2020.proxima_comb,
                parts_2020.proxima_power,
                parts_2020.div_position,
            ),
            description=u"proxima - основная офлайн метрика ранжирования (со степенью)",
            revision=REVISION_2020,
            wiki=WIKI,
        ),
        ProximaDescription(
            name="proxima2020-linear",
            parts=(
                parts_2020.proxima_rel,
                parts_2020.proxima_comb,
                parts_2020.div_position,
            ),
            description=u"proxima - основная офлайн метрика ранжирования (без степени)",
            revision=REVISION_2020,
            wiki=WIKI,
        ),
        ProximaDescription(
            name="proxima2020-weight",
            parts=(
                parts_2020.proxima_rel,
                parts_2020.proxima_comb,
                parts_2020.proxima_weight,
                parts_2020.div_position,
            ),
            description=u"вес запроса в proxima или отношение proxima2020 / proxima2020-linear",
            revision=REVISION_2020,
            wiki=WIKI,
        ),
    ])

    proxima_light = ProximaDescription(
        name="proxima2020-light",
        parts=(
            parts_2020.proxima_sinsig_light,
            parts_2020.proxima_comb,
            parts_2020.proxima_power,
            parts_2020.div_position,
        ),
        description=u"proxima без dbd для свежести (со степенью)",
        revision=REVISION_2020,
        wiki=WIKI,
    )
    proxima_light_ignore_empty_serp = proxima_light.copy()
    proxima_light_ignore_empty_serp.name = "proxima2020-light-ignore-empty-serps"
    parts_2020.webfresh_ignore_empty_serp(proxima_light_ignore_empty_serp.part)
    proxima_svin = ProximaDescription(
        name="proxima2020-light-svin",
        parts=(
            parts_2020.proxima_sinsig_svin,
            parts_2020.proxima_comb,
            parts_2020.proxima_power,
            parts_2020.div_position,
        ),
        description=u"proxima без dbd с бонусом за свежесть (со степенью)",
        revision=REVISION_2020,
        wiki=WIKI,
    )
    proxima_svin.part.scales["sinsig_kc_no_turbo_judgement_values"] = parts_2020.SINSIG_SVIN_SCALE
    proxima_svin_ignore_empty_serp = proxima_svin.copy()
    proxima_svin_ignore_empty_serp.name = "proxima2020-light-svin-ignore-empty-serps"
    parts_2020.webfresh_ignore_empty_serp(proxima_svin_ignore_empty_serp.part)

    metrics.extend([
        proxima_light,
        proxima_light_ignore_empty_serp,
        proxima_svin,
        proxima_svin_ignore_empty_serp,
    ])

    metrics.extend([
        ProximaDescription(
            name="proxima2020-sinsig",
            parts=(
                parts_2020.proxima_sinsig,
                parts_2020.div_position,
            ),
            description=u"только sinsig из proxima (без степени)",
            revision=REVISION_2020,
            wiki=WIKI,
        ),
        ProximaDescription(
            name="proxima2020-updbd",
            parts=(
                parts_2020.proxima_updbd,
                parts_2020.div_position,
            ),
            description=u"только updbd из proxima (без степени)",
            revision=REVISION_2020,
            wiki=WIKI,
        ),
        ProximaDescription(
            name="proxima2020-rel",
            parts=(
                parts_2020.proxima_rel,
                parts_2020.div_position,
            ),
            description=u"sinsig+updbd из proxima (без степени)",
            revision=REVISION_2020,
            wiki=WIKI,
        ),
        ProximaDescription(
            name="proxima2020-comb",
            parts=(
                parts_2020.proxima_comb,
                parts_2020.div_position,
            ),
            description=u"только комбинированный сигнал от антиспама из proxima (без степени)",
            revision=REVISION_2020,
            wiki=WIKI,
        ),
        ProximaDescription(
            name="proxima2020-fine-ungrouping",
            parts=(
                parts_2020.fine_ungrouping_single,
                parts_2020.div_position,
            ),
            description=u"штраф за разгруппировку из proxima (dcg)",
            revision=REVISION_2020,
        ),
        ProximaDescription(
            name="proxima2020-fine-dup",
            parts=(
                parts_2020.fine_dup_single,
                parts_2020.div_position,
            ),
            description=u"штраф за дубли из proxima (dcg)",
            revision=REVISION_2020,
            wiki=WIKI,
        ),
        ProximaDescription(
            name="proxima2020-fine-super-dup",
            parts=(
                parts_2020.fine_super_dup_single,
                parts_2020.div_position,
            ),
            description=u"штраф за полные дубли из proxima (dcg)",
            revision=REVISION_2020,
            wiki=WIKI,
        ),
        ProximaDescription(
            name="proxima2020-fine-plag",
            parts=(
                parts_2020.fine_plag_single,
                parts_2020.div_position,
            ),
            description=u"штраф за плагиатность из proxima (dcg)",
            revision=REVISION_2020,
            wiki=WIKI,
        ),
        ProximaDescription(
            name="proxima2020-fine-full",
            parts=(
                parts_2020.fine_full_single,
                parts_2020.div_position,
            ),
            description=u"полный штраф из proxima (dcg)",
            revision=REVISION_2020,
            wiki=WIKI,
        ),
        ProximaDescription(
            name="proxima2020-fine-host",
            parts=(
                parts_2020.fine_host_single,
                parts_2020.div_position,
            ),
            description=u"хостовый штраф из proxima (dcg)",
            revision=REVISION_2020,
            wiki=WIKI,
        ),
    ])

    metrics.extend([
        ProximaDescription(
            name="ungrouping-dcg-5",
            parts=(
                parts_2020.ungrouping_single,
                parts_2020.div_position,
            ),
            revision=REVISION_2020,
            greater_better=False,
        ),
        ProximaDescription(
            name="plag-dcg-5",
            parts=(
                parts_2020.plag_single,
                parts_2020.div_position,
            ),
            revision=REVISION_2020,
            greater_better=False,
        ),
        ProximaDescription(
            name="plag-dup-dcg-5",
            parts=(
                parts_2020.plag_dup_single,
                parts_2020.div_position,
            ),
            revision=REVISION_2020,
            greater_better=False,
        ),
        ProximaDescription(
            name="plag-super-dup-dcg-5",
            parts=(
                parts_2020.plag_super_dup_single,
                parts_2020.div_position,
            ),
            revision=REVISION_2020,
            greater_better=False,
        ),
        ProximaDescription(
            name="updbd-toloka-dcg-5",
            parts=(
                parts_2020.updbd_single,
                parts_2020.div_position,
            ),
            revision=REVISION_2020,
        ),
        ProximaDescription(
            name="comb-host-dcg-5",
            parts=(
                parts_2020.comb_single,
                parts_2020.div_position,
            ),
            revision=REVISION_2020,
        ),
        ProximaDescription(
            name="sinsig-kc-no-turbo-dcg-5",
            parts=(
                parts_2020.sinsig_single,
                parts_2020.div_position,
            ),
            description=u"Чистый sinsig-dcg (без штрафов)",
            revision=REVISION_2020,
        ),
        ProximaDescription(
            name="sinsig-marker-5",
            parts=(
                parts_2020.sinsig_marker,
            ),
            description=u"Наличие документа лучше чем слайдер (OFFLINE-593)",
            revision=REVISION_2020,
        ),
        ProximaDescription(
            name="sinsig-kc-no-turbo-sum-5",
            parts=(
                parts_2020.sinsig_single,
            ),
            description=u"Сумма sinsig (без штрафов)",
            revision=REVISION_2020,
        ),
        ProximaDescription(
            name="sinsig-kc-no-turbo-vital-dcg-5",
            parts=(
                parts_2020.sinsig_vital_single,
                parts_2020.div_position,
            ),
            description=u"Чистый sinsig-dcg (без штрафов), но только витальники",
            revision=REVISION_2020,
        ),
        ProximaDescription(
            name="sinsig-kc-no-turbo-nonvital-dcg-5",
            parts=(
                parts_2020.sinsig_nonvital_single,
                parts_2020.div_position,
            ),
            description=u"Чистый sinsig-dcg (без штрафов), но без витальников",
            revision=REVISION_2020,
        ),
    ])

    metrics.extend([
        ProximaDescription(
            name="proxima2020ecomm",
            parts=(
                parts_2020.proxima_rel,
                parts_2020.proxima_ecomm_comb,
                parts_2020.proxima_ecomm_comm,
                parts_2020.proxima_power,
                parts_2020.div_position,
            ),
            description=u"коммерческая проксима",
            revision=REVISION_2020,
            wiki=WIKI,
        ),
        ProximaDescription(
            name="proxima2020ecomm-full",
            parts=(
                parts_2020.proxima_rel,
                parts_2020.proxima_ecomm_comb,
                parts_2020.proxima_ecomm_comm_full,
                parts_2020.proxima_power,
                parts_2020.div_position,
            ),
            description=u"коммерческая проксима без учета pay_detector",
            revision=REVISION_2020,
            wiki=WIKI,
        ),
        ProximaDescription(
            name="proxima2020ecomm-linear",
            parts=(
                parts_2020.proxima_rel,
                parts_2020.proxima_ecomm_comb,
                parts_2020.proxima_ecomm_comm,
                parts_2020.div_position,
            ),
            description=u"коммерческая проксима (без степени)",
            revision=REVISION_2020,
            wiki=WIKI,
        ),
        ProximaDescription(
            name="proxima2020ecomm-comm",
            parts=(
                parts_2020.proxima_ecomm_comm,
                parts_2020.div_position,
            ),
            description=u"коммерческая проксима (только коммерческая часть)",
            revision=REVISION_2020,
            wiki=WIKI,
        ),
        ProximaDescription(
            name="proxima2020ecomm-comb",
            parts=(
                parts_2020.proxima_ecomm_comb,
                parts_2020.div_position,
            ),
            description=u"коммерческая проксима (только сигнал антиспама)",
            revision=REVISION_2020,
            wiki=WIKI,
        ),
        ProximaDescription(
            name="page-marketability-dcg-5",
            parts=(
                parts_2020.variety_single,
                parts_2020.div_position,
            ),
            description=u"marketability или variety",
            revision=REVISION_2020,
            wiki=WIKI,
        ),
        ProximaDescription(
            name="product-availability-dcg-5",
            parts=(
                parts_2020.in_stock_single,
                parts_2020.div_position,
            ),
            description=u"availability или in_stock",
            revision=REVISION_2020,
            wiki=WIKI,
        ),

        ProximaDescription(
            name="ecom-prices-competitiveness-dcg-5",
            parts=(
                parts_2020.ecom_prices_single,
                parts_2020.div_position,
            ),
            description=u"Сигнал цены",
            revision=REVISION_2020,
            wiki=WIKI,
        ),
        ProximaDescription(
            name="ecom-assortment-dcg-5",
            parts=(
                parts_2020.ecom_assortment_single,
                parts_2020.div_position,
            ),
            description=u"Сигнал широты ассортимента",
            revision=REVISION_2020,
            wiki=WIKI,
        ),
        ProximaDescription(
            name="ecom-host-quality-dcg-5",
            parts=(
                parts_2020.ecom_host_quality_single,
                parts_2020.div_position,
            ),
            description=u"Коммерческое качество хоста",
            revision=REVISION_2020,
            wiki=WIKI,
        ),
        ProximaDescription(
            name="is-actual-seller-dcg-5",
            parts=(
                parts_2020.is_actual_seller_single,
                parts_2020.div_position,
            ),
            description=u"f_type_plus_service == 1",
            revision=REVISION_2020,
            wiki=WIKI,
        ),
        ProximaDescription(
            name="ecom-kernel-vers2-dcg-5",
            parts=(
                parts_2020.ecom_kernel_v2_single,
                parts_2020.div_position,
            ),
            description=u"Коммерческий кернел, вторая версия",
            revision=REVISION_2020,
            wiki=WIKI,
        ),
    ])

    metrics.extend([
        ProximaDescription(
            name="comm-normalized-5",
            parts=(
                parts_2020.proxima_ecomm_comm,
                parts_2020.proxima_ecomm_comm_sinsig_normalize
            ),
            description="comm part normalized",
            revision=REVISION_2020,
            wiki=WIKI
        )
    ])

    assert len({m.name for m in metrics}) == len(metrics), "some metrics have identical names"
    metrics.sort(key=lambda m: m.name)
    return metrics


def main():
    cli_args = mc_common.parse_args()
    umisc.configure_logger()

    proximas = generate_proxima()
    mc_common.save_generated(proximas, cli_args)


if __name__ == "__main__":
    main()
