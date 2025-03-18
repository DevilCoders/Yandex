# coding=utf-8
from proxima_description import ProximaPart

SINSIG = (
    "avg(L.sinsig_kc_no_turbo_judgement_values) + 1.2 * avg([max(0, slider) "
    "/ 100.0 for slider in L.sinsig_kc_no_turbo_slider_values])"
)

SINSIG_SCALE = {
    "404": 0,
    "SLIDER_GRADE": 0.25,
    "IRREL": 0,
    "RELEVANCE_MINUS_GOOD": 0.25,
    "RELEVANCE_MINUS_BAD": 0.12,
    "VITAL_GOOD": 2,
    "SUBVITAL": 1.3,
    "VITAL_BAD": 1,
    "__DEFAULT__": None,
}

SINSIG_SVIN_SCALE = {
    "404": 0,
    "SLIDER_GRADE": 0.25,
    "IRREL": 0,
    "RELEVANCE_MINUS_GOOD": 0.25,
    "RELEVANCE_MINUS_BAD": 0.12,
    "VITAL_GOOD": 2,
    "SUBVITAL": 1.01,
    "VITAL_BAD": 1,
    "__DEFAULT__": None,
}

FINE_UNGROUPING = (
    "0.9 ** S.ungrouping_count_no_turbo "
    "if int(serp_data.get('query_param.need_certain_host', 0)) "
    "else 0.7 ** S.ungrouping_count_no_turbo"
)
FINE_SUPER_DUP = "0.1 ** S.super_dup_fine"
FINE_PLAG = "0.6 ** S.plag_fine"
FINE_DUP = "0.8 ** S.dup_fine"

FULL_FINE_AP = (
    "D.custom_formulas['fine_ungrouping'] * D.custom_formulas['fine_dup'] * "
    "D.custom_formulas['fine_super_dup'] * D.custom_formulas['fine_plag'] "
    "if D.custom_formulas['rel'] > 0 "
    "else 1.0"
)
FULL_FINE_AP_LIGHT = (
    "D.custom_formulas['fine_ungrouping'] * D.custom_formulas['fine_dup'] * "
    "D.custom_formulas['fine_super_dup'] * D.custom_formulas['fine_plag']"
)

HOST_FINE_AP = (
    "D.custom_formulas['fine_ungrouping'] * D.custom_formulas['fine_plag'] "
    "if D.custom_formulas['comb'] > 0 "
    "else 1.0"
)

DUP_PLAG_CONFIDENCE = {
    "name": "dup-plag-counts-judged-5",
    "threshold": 0.95,
    "condition": "GREATER",
    "requirements": [],
    "regional": None,
    "yandex": False,
}

SVIN_INTENT = "1.0 if (int(serp_data.get('query_param.intent')) <= 20) else 1.4"
SVIN_NEWS = "int(serp_data.get('query_param.need_news') == 'true') * int(S.fresh_top_news_score >= 0.1)"

base = ProximaPart(
    confidences=[
        {
            "name": "serp-failed",
            "threshold": 0.003,
            "condition": "SMALLER",
            "requirements": [],
            "regional": None,
            "yandex": False,
        },
        {
            "name": "empty-serp",
            "threshold": 0.01,
            "condition": "SMALLER",
            "requirements": [],
            "regional": None,
            "yandex": False,
        },
        {
            "name": "empty-serp",
            "threshold": 0.001,
            "condition": "SMALLER",
            "requirements": [],
            "regional": None,
            "yandex": True,
        },
    ],
)

div_position = ProximaPart(divisor="S.position")

ungrouping_single = base.copy()
ungrouping_single.label_script_parts = ["S.ungrouping_count_no_turbo"]

plag_dup = base.copy()
plag_dup.confidences.append(DUP_PLAG_CONFIDENCE)
plag_dup.requirements.add("COMPONENT.judgements.dup_fine")
plag_dup.signals.add("dup_fine")

plag_super_dup = base.copy()
plag_super_dup.confidences.append(DUP_PLAG_CONFIDENCE)
plag_super_dup.requirements.add("COMPONENT.judgements.super_dup_fine")
plag_super_dup.signals.add("super_dup_fine")

plag = base.copy()
plag.confidences.append(DUP_PLAG_CONFIDENCE)
plag.requirements.add("COMPONENT.judgements.plag_fine")
plag.signals.add("plag_fine")

fine_ungrouping = base.copy()
fine_ungrouping.requirements.add("SERP.query_param.need_certain_host")
fine_ungrouping.custom_formulas["fine_ungrouping"] = FINE_UNGROUPING

fine_dup = plag_dup.copy()
fine_dup.custom_formulas["fine_dup"] = FINE_DUP

fine_super_dup = plag_super_dup.copy()
fine_super_dup.custom_formulas["fine_super_dup"] = FINE_SUPER_DUP

fine_plag = plag.copy()
fine_plag.custom_formulas["fine_plag"] = FINE_PLAG

plag_dup_single = plag_dup.copy()
plag_dup_single.label_script_parts = ["S.dup_fine"]

plag_super_dup_single = plag_super_dup.copy()
plag_super_dup_single.label_script_parts = ["S.super_dup_fine"]

plag_single = plag.copy()
plag_single.label_script_parts = ["S.plag_fine"]

fine_ungrouping_single = fine_ungrouping.copy()
fine_ungrouping_single.label_script_parts = ["D.custom_formulas['fine_ungrouping']"]

fine_dup_single = fine_dup.copy()
fine_dup_single.label_script_parts = ["D.custom_formulas['fine_dup']"]

fine_super_dup_single = fine_super_dup.copy()
fine_super_dup_single.label_script_parts = ["D.custom_formulas['fine_super_dup']"]

fine_plag_single = fine_plag.copy()
fine_plag_single.label_script_parts = ["D.custom_formulas['fine_plag']"]

sinsig = base.copy()
sinsig.confidences.append({
    "name": "judged-sinsig-kc-no-turbo-judgement-values-5",
    "threshold": 0.999,
    "condition": "GREATER",
    "requirements": [],
    "regional": None,
    "yandex": False,
})
sinsig.requirements.update({
    "COMPONENT.judgements.sinsig_kc_no_turbo_judgement_values",
    "COMPONENT.judgements.sinsig_kc_no_turbo_slider_values",
})
sinsig.custom_formulas["sinsig"] = SINSIG
sinsig.signals.update({
    "sinsig_kc_no_turbo_judgement_values",
    "sinsig_kc_no_turbo_slider_values",
})
sinsig.scales["sinsig_kc_no_turbo_judgement_values"] = SINSIG_SCALE

updbd = base.copy()
updbd.confidences.append({
    "name": "judged-updbd-toloka-eval-bt-score-5",
    "threshold": 0.995,
    "condition": "GREATER",
    "requirements": [],
    "regional": None,
    "yandex": False,
})
updbd.requirements.add("COMPONENT.judgements.updbd_toloka_eval_bt_score")
updbd.custom_formulas["updbd"] = "S.updbd_toloka_eval_bt_score"
updbd.signals.add("updbd_toloka_eval_bt_score")

rel = ProximaPart.merge_list([sinsig, updbd])
rel.custom_formulas["rel"] = "0.69 * ({}) + 0.56/4 * (S.updbd_toloka_eval_bt_score + 2.0)".format(SINSIG)
del rel.custom_formulas["sinsig"]
del rel.custom_formulas["updbd"]

comb = base.copy()
comb.confidences.append({
    "name": "comb-host-signal-judged-5",
    "threshold": 0.92,
    "condition": "GREATER",
    "requirements": [],
    "regional": None,
    "yandex": False,
}
)
comb.requirements.add("COMPONENT.judgements.comb_host_signal_enrichment")
comb.custom_formulas["comb"] = "S.Host_signal - 0.5 if 'Host_signal' in S else 0.0"
comb.signals.add("comb_host_signal_enrichment")

comb_with_sinsig = ProximaPart.merge_list([comb, sinsig])
comb_with_sinsig.custom_formulas["comb"] = (
    "S.Host_signal - 0.5 if 'Host_signal' in S"
    " and (avg(L.sinsig_kc_no_turbo_judgement_values) > 0 or S.Host_signal < 0.5)"
    " else 0.0"
)
del comb_with_sinsig.custom_formulas["sinsig"]

sinsig_single = sinsig.copy()
sinsig_single.label_script_parts = ["D.custom_formulas['sinsig']"]

sinsig_marker = sinsig.copy()
sinsig_marker.label_script_parts = ["1 if D.custom_formulas['sinsig'] >= 1.0 else 0"]
sinsig_marker.aggregate_script = "max(position_results) if position_results else 0"
sinsig_marker.update_confidence("judged-sinsig-kc-no-turbo-judgement-values-5", 0.99)

sinsig_vital_single = sinsig.copy()
sinsig_vital_single.label_script_parts = ["D.custom_formulas['sinsig'] if D.custom_formulas['sinsig'] >= 1.5 else 0.0"]

sinsig_nonvital_single = sinsig.copy()
sinsig_nonvital_single.label_script_parts = [
    "D.custom_formulas['sinsig'] if D.custom_formulas['sinsig'] < 1.5 else 0.0",
]

updbd_single = updbd.copy()
updbd_single.label_script_parts = ["D.custom_formulas['updbd']"]

comb_single = comb.copy()
comb_single.label_script_parts = ["D.custom_formulas['comb']"]

fine_host = ProximaPart.merge_list([
    fine_ungrouping,
    fine_plag,
    comb_with_sinsig,
])
fine_host.custom_formulas_ap["fine_host"] = HOST_FINE_AP

fine_full = ProximaPart.merge_list([
    fine_ungrouping,
    fine_dup,
    fine_super_dup,
    fine_plag,
    rel,
])
fine_full.custom_formulas_ap["fine_full"] = FULL_FINE_AP

fine_full_light = ProximaPart.merge_list([
    fine_ungrouping,
    fine_dup,
    fine_super_dup,
    fine_plag,
    sinsig,
])
fine_full_light.custom_formulas_ap["fine_full"] = FULL_FINE_AP_LIGHT

fine_host_single = fine_host.copy()
fine_host_single.label_script_parts = ["D.custom_formulas['fine_host']"]

fine_full_single = fine_full.copy()
fine_full_single.label_script_parts = ["D.custom_formulas['fine_full']"]

proxima_sinsig = ProximaPart.merge_list([sinsig, fine_full])
proxima_sinsig.label_script_parts = ["0.69 * D.custom_formulas['sinsig'] * D.custom_formulas['fine_full']"]

proxima_sinsig_light = ProximaPart.merge_list([sinsig, fine_full_light])
proxima_sinsig_light.label_script_parts = ["0.69 * D.custom_formulas['sinsig'] * D.custom_formulas['fine_full']"]

proxima_updbd = ProximaPart.merge_list([updbd, fine_full])
proxima_updbd.custom_formulas["updbd"] = "S.updbd_toloka_eval_bt_score + 2.0"
proxima_updbd.label_script_parts = ["0.56/4 * D.custom_formulas['updbd'] * D.custom_formulas['fine_full']"]

proxima_rel = ProximaPart.merge_list([rel, fine_full])
proxima_rel.label_script_parts = ["1.0 * D.custom_formulas['rel'] * D.custom_formulas['fine_full']"]

comb_with_sinsig_and_cluster = comb_with_sinsig.copy()
comb_with_sinsig_and_cluster.custom_formulas[
    "cluster_boost"] = "1.8 if serp_data.get('query_param.cluster') == '34' else 1.0"
comb_with_sinsig_and_cluster.requirements.add("SERP.query_param.cluster")

proxima_comb = ProximaPart.merge_list([comb_with_sinsig_and_cluster, fine_host])
proxima_comb.label_script_parts = [
    "0.15/0.882 * D.custom_formulas['comb'] * D.custom_formulas['fine_host'] * D.custom_formulas['cluster_boost']",
]

POWER_07 = "power_positive(sum(position_results), 1 if any(D.all_custom_formulas['is_vital'][:5]) else 0.7)"
WEIGHT_07 = "1.0 if sum(position_results) == 0 else min(2.0, {} / sum(position_results))".format(POWER_07)

proxima_power = ProximaPart(
    custom_formulas={
        "is_vital": "{} >= 1.5".format(SINSIG),
    },
    aggregate_script=POWER_07,
)
proxima_weight = proxima_power.copy()
proxima_weight.aggregate_script = WEIGHT_07

proxima_sinsig_svin = proxima_sinsig_light.copy()
proxima_sinsig_svin.label_script_parts = ["0.69 * D.custom_formulas['sinsig_svin'] * D.custom_formulas['fine_full']"]
proxima_sinsig_svin.custom_formulas["news_boost"] = SVIN_NEWS
proxima_sinsig_svin.custom_formulas["intent_boost"] = SVIN_INTENT
proxima_sinsig_svin.requirements.update({
    "COMPONENT.judgements.fresh_top_news_score",
    "SERP.query_param.intent",
    "SERP.query_param.need_news",
})
proxima_sinsig_svin.signals.add("fresh_top_news_score")
proxima_sinsig_svin.custom_formulas_ap["sinsig_svin"] = (
    "D.custom_formulas['sinsig'] + "
    "0.13 * (D.custom_formulas['sinsig'] >= 0.4) * D.custom_formulas['news_boost'] * D.custom_formulas['intent_boost']"
)


def webfresh_ignore_empty_serp(part):
    part.update_confidence("empty-serp", 0.9)
    skip_empty_prefix = "None if not position_results else "
    if not part.aggregate_script.startswith(skip_empty_prefix):
        aggregate_script = part.aggregate_script or "sum(position_results)"
        part.aggregate_script = skip_empty_prefix + aggregate_script


def wizard_dup_plag_conf(part):
    part.update_confidence("dup-plag-counts-judged-5", 0.8)


def wizard_impact(part, part_impact):
    part.merge_with(part_impact)
    assert not part.aggregate_script
    part.aggregate_script = (
        "sum(value/(pos+1) for pos, value in enumerate(all_custom_formulas['value']) if pos<5)"
        " - sum(value/(pos+1) for pos, value in enumerate(value for value, is_target in"
        " zip(all_custom_formulas['value'],all_custom_formulas['is_target_wizard']) if not is_target) if pos<5)"
    )
    part.label_script_parts = ["D.custom_formulas['value']"]
    part.custom_formulas_ap["value"] = (
        "0.69 * D.custom_formulas['sinsig'] "
        "+ 0.15/0.882 * D.custom_formulas['comb'] * D.custom_formulas['cluster_boost']"
    )


def cluster(part, cluster_number):
    prefix = "None if serp_data.get('query_param.cluster') != '{:d}' else ".format(cluster_number)
    if not part.aggregate_script.startswith(prefix):
        aggregate_script = part.aggregate_script or "sum(position_results)"
        part.aggregate_script = prefix + aggregate_script
    part.requirements.add("SERP.query_param.cluster")


albin_url = base.copy()
albin_url.confidences.append({
    "name": "albin-url-signals-old-judged-5",
    "threshold": 0.8,
    "condition": "GREATER",
    "requirements": [],
    "regional": None,
    "yandex": False,
})
albin_url.requirements.update({
    "COMPONENT.judgements.albin_url_bundle_old",
})
albin_url.signals.update({
    "albin_url_bundle_old",
})

prgg = albin_url.copy()
prgg.custom_formulas["prgg"] = "float(S.prgg)"
prgg.label_script_parts = ["1.0 * D.custom_formulas['prgg']"]

in_stock = base.copy()
in_stock.confidences.append({
    "name": "judged-product-availability-5",
    "threshold": 0.99,
    "condition": "GREATER",
    "requirements": [],
    "regional": None,
    "yandex": False,
})
in_stock.requirements.update({
    "COMPONENT.judgements.product_availability_eval_url_results",
})
in_stock.signals.update({
    "product_availability_eval_url_results",
})
in_stock.custom_formulas["availability"] = "avg(L.product_availability_eval_url_results)"
in_stock.scales["product_availability_eval_url_results"] = {
    "PRODUCT_AVAILABLE": 1,
    "SERVICE_AVAILABLE": 1,
    "OTHER_AVAILABLE": 1,
    "PRODUCT_NOT_AVAILABLE": 0,
    "SERVICE_NOT_AVAILABLE": 0,
    "OTHER_NOT_AVAILABLE": 0,
}

in_stock_single = in_stock.copy()
in_stock_single.label_script_parts = ["D.custom_formulas['availability']"]

variety = base.copy()
variety.confidences.append({
    "name": "judged-page-marketability-eval-5",
    "threshold": 0.995,
    "condition": "GREATER",
    "requirements": [],
    "regional": None,
    "yandex": False,
})
variety.requirements.update({
    "COMPONENT.judgements.page_marketability_eval_results",
})
variety.signals.update({
    "page_marketability_eval_results",
})
variety.custom_formulas["page_marketability"] = "avg(L.page_marketability_eval_results)"
variety.scales["page_marketability_eval_results"] = {
    "MANY_PRODUCTS": 1,
    "ONE_PRODUCT": 0.5,
    "NO_PRODUCTS": -1,
}

variety_single = variety.copy()
variety_single.label_script_parts = ["D.custom_formulas['page_marketability']"]

ecom_kernel_v2 = base.copy()
ecom_kernel_v2.confidences.append({
    "name": "ecom-kernel-vers2-judged-5",
    "threshold": 0.9,
    "condition": "GREATER",
    "requirements": [],
    "regional": None,
    "yandex": False,
})
ecom_kernel_v2.requirements.update({
    "COMPONENT.judgements.ecom_biz_kernel_enrichment2",
})
ecom_kernel_v2.signals.update({
    "ecom_biz_kernel_enrichment2",
})
ecom_kernel_v2.custom_formulas["ecomm_kernel_ugc"] = "S.ppEcom_kernel - 0.5 if 'ppEcom_kernel' in S else 0"

ecom_kernel_v2_single = ecom_kernel_v2.copy()
ecom_kernel_v2_single.label_script_parts = ["D.custom_formulas['ecomm_kernel_ugc']"]

ecom_assortment = base.copy()
ecom_assortment.confidences.extend([
    {
        "name": "ecom-assortment-judged-5",
        "threshold": 0.3,
        "condition": "GREATER",
        "requirements": [],
        "regional": None,
        "yandex": False,
    },
])
ecom_assortment.requirements.update({
    "COMPONENT.judgements.ecom_assortment",
})
ecom_assortment.signals.update({
    "ecom_assortment",
})
ecom_assortment.custom_formulas["assortment"] = "S.assortment_share - 0.2 if 'assortment_share' in S else 0"

ecom_assortment_single = ecom_assortment.copy()
ecom_assortment_single.label_script_parts = ["D.custom_formulas['assortment']"]

ecom_host_quality_req = base.copy()
ecom_host_quality_req.confidences.extend([
    {
        "name": "ecom-host-quality-judged-5",
        "threshold": 0.3,
        "condition": "GREATER",
        "requirements": [],
        "regional": None,
        "yandex": False,
    },
])
ecom_host_quality_req.requirements.update({
    "COMPONENT.judgements.ecom_host_quality_enrichment",
})
ecom_host_quality_req.signals.update({
    "ecom_host_quality_enrichment",
})

ecom_host_quality = ecom_host_quality_req.copy()
ecom_host_quality.custom_formulas["ecom_host_quality"] = (
    "S.host_comm_product - 2.5 if 'host_comm_product' in S else 0"
)

ecom_host_quality_single = ecom_host_quality.copy()
ecom_host_quality_single.label_script_parts = ["D.custom_formulas['ecom_host_quality']"]

is_actual_seller = ecom_host_quality_req.copy()
is_actual_seller.custom_formulas["is_actual_seller"] = "float(S.f_type_plus_service == 1)"

is_actual_seller_single = is_actual_seller.copy()
is_actual_seller_single.label_script_parts = ["D.custom_formulas['is_actual_seller']"]

pay_detector = base.copy()
pay_detector.requirements.add("SERP.query_param.pay_detector")
pay_detector.custom_formulas["is_highly_commercial"] = (
    "float(serp_data.get('query_param.pay_detector', 0.6)) > 0.9"
)

ecom_prices = base.copy()
ecom_prices.confidences.extend([
    {
        "name": "ecom-prices-judged-5",
        "threshold": 0.3,
        "condition": "GREATER",
        "requirements": [],
        "regional": None,
        "yandex": False,
    },
])
ecom_prices.requirements.update({
    "COMPONENT.judgements.ecom_prices_competitiveness",
})
ecom_prices.signals.update({
    "ecom_prices_competitiveness",
})
ecom_prices.custom_formulas["price"] = "1 - S.price_percentile * 0.01 - 0.4 if 'price_percentile' in S else 0"

ecom_prices_single = ecom_prices.copy()
ecom_prices_single.label_script_parts = ["D.custom_formulas['price']"]

proxima_ecomm_comm_full = ProximaPart.merge_list([
    sinsig,
    fine_super_dup,
    fine_ungrouping,
    variety,
    is_actual_seller,
    ecom_host_quality,
    ecom_assortment,
    ecom_prices,
    ecom_kernel_v2,
])
proxima_ecomm_comm_full.custom_formulas_ap["comm"] = (
    "("
    "0.09 * D.custom_formulas['page_marketability']"
    " * D.custom_formulas['is_actual_seller']"
    " * D.custom_formulas['fine_super_dup']"
    " + 0.035 / 0.3 * D.custom_formulas['ecomm_kernel_ugc']"
    " + 0.15 * (0.62 * D.custom_formulas['price'] + 0.38 * D.custom_formulas['assortment'])"
    " + 0.065 / 0.25 * D.custom_formulas['ecom_host_quality']"
    " - 0.09 * D.custom_formulas['is_actual_seller']"
    ")"
    " * D.custom_formulas['fine_ungrouping']"
    " * "
    "("
    "0.05 * (D.custom_formulas['sinsig'] <= 0.25) * D.custom_formulas['sinsig'] / 0.25"
    " + 0.7 * (0.25 < D.custom_formulas['sinsig'] <= 0.55)"
    " + 1. * (0.55 < D.custom_formulas['sinsig'] <= 0.85)"
    " + 1.2 * (0.85 < D.custom_formulas['sinsig'])"
    ")"
)
proxima_ecomm_comm_full.label_script_parts = ["1.0 * D.custom_formulas['comm'] * D.custom_formulas['is_actual_seller']"]
proxima_ecomm_comm_full.confidences.extend([
    {
        "name": "ecom-commercial-count-5",
        "threshold": 1.0,
        "condition": "GREATER",
        "requirements": [],
        "regional": None,
        "yandex": False,
    },
    {
        "name": "ecom-host-quality-judged-normed-5",
        "threshold": 0.95,
        "condition": "GREATER",
        "requirements": [],
        "regional": None,
        "yandex": False,
    },
    {
        "name": "ecom-assortment-judged-normed-5",
        "threshold": 0.7,
        "condition": "GREATER",
        "requirements": [],
        "regional": None,
        "yandex": False,
    },
    {
        "name": "ecom-prices-judged-normed-5",
        "threshold": 0.7,
        "condition": "GREATER",
        "requirements": [],
        "regional": None,
        "yandex": False,
    },
])

proxima_ecomm_comm = ProximaPart.merge_list([proxima_ecomm_comm_full, pay_detector])
proxima_ecomm_comm.custom_formulas_ap["comm"] = (
    "("
    "0.09 * D.custom_formulas['page_marketability']"
    " * D.custom_formulas['is_actual_seller']"
    " * D.custom_formulas['fine_super_dup']"
    " + 0.035 / 0.3 * D.custom_formulas['ecomm_kernel_ugc']"
    " + 0.15 * (0.62 * D.custom_formulas['price'] + 0.38 * D.custom_formulas['assortment'])"
    " + 0.065 / 0.25 * D.custom_formulas['ecom_host_quality']"
    " - 0.09 * D.custom_formulas['is_actual_seller']"
    ")"
    " * D.custom_formulas['fine_ungrouping']"
    " * (0.5 + 0.5 * D.custom_formulas['is_highly_commercial'])"
    " * "
    "("
    "0.05 * (D.custom_formulas['sinsig'] <= 0.25) * D.custom_formulas['sinsig'] / 0.25"
    " + 0.7 * (0.25 < D.custom_formulas['sinsig'] <= 0.55)"
    " + 1. * (0.55 < D.custom_formulas['sinsig'] <= 0.85)"
    " + 1.2 * (0.85 < D.custom_formulas['sinsig'])"
    ")"
)

proxima_ecomm_comb = ProximaPart.merge_list([proxima_comb, is_actual_seller])
proxima_ecomm_comb.custom_formulas["comb"] = (
    "S.Host_signal - 0.76 if 'Host_signal' in S"
    " and (avg(L.sinsig_kc_no_turbo_judgement_values) > 0 or S.Host_signal < 0.5)"
    " else 0.0"
)
proxima_ecomm_comb.label_script_parts = [
    "0.15/0.882 * D.custom_formulas['comb'] * D.custom_formulas['fine_host'] * D.custom_formulas['cluster_boost'] "
    "* (1 - D.custom_formulas['is_actual_seller'])",
]

proxima_ecomm_comm_sinsig_normalize = ProximaPart.merge_list([sinsig])
proxima_ecomm_comm_sinsig_normalize.custom_formulas_ap["comm_sinsig_normalize"] = (
    "("
    "0.05 * (D.custom_formulas['sinsig'] <= 0.25) * D.custom_formulas['sinsig'] / 0.25"
    " + 0.7 * (0.25 < D.custom_formulas['sinsig'] <= 0.55)"
    " + 1. * (0.55 < D.custom_formulas['sinsig'] <= 0.85)"
    " + 1.2 * (0.85 < D.custom_formulas['sinsig'])"
    ")"
)
proxima_ecomm_comm_sinsig_normalize.aggregate_script = (
    "sum(["
    "result / (index + 1) "
    "for index, result in enumerate("
    "[x for x, y in zip(D.all_custom_formulas['comm'][:5], D.all_custom_formulas['is_actual_seller'][:5]) if y]"
    ")"
    "])"
    " / (1e-5 + sum(["
    "result / (index + 1) "
    "for index, result in enumerate("
    "[x for x, y in zip(D.all_custom_formulas['comm_sinsig_normalize'][:5], D.all_custom_formulas['is_actual_seller'][:5]) if y]"
    ")]))"
)
