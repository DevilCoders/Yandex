# coding=utf-8
from proxima_description import ProximaPart

SINSIG = (
    "avg(L.sinsig_kc_judgement_values) + 1.2 * avg([max(0, slider) "
    "/ 100.0 for slider in L.sinsig_kc_slider_values])"
)
SINSIG_NO_TURBO = (
    "avg(L.sinsig_kc_no_turbo_judgement_values) + 1.2 * avg([max(0, slider) "
    "/ 100.0 for slider in L.sinsig_kc_no_turbo_slider_values])"
)

MOBILE_SCALE = {
    "0": 0,
    "1": 1,
    "DESKTOP": 0,
    "__SKIP_NOT_JUDGED__": False,
}

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

SINSIG_BA_SCALE = {
    "404": 0,
    "SLIDER_GRADE": 0.25,
    "IRREL": 0,
    "RELEVANCE_MINUS_GOOD": 0.25,
    "RELEVANCE_MINUS_BAD": 0.12,
    "VITAL_GOOD": 0.85,
    "SUBVITAL": 0.85,
    "VITAL_BAD": 0.85,
    "__DEFAULT__": None,
}

FINE_DUP = "0.1 ** S.full_duplicates_count * 0.75 ** S.text_duplicates_count"
FINE_UNGROUPING = (
    "0.9 ** S.ungrouping_count_no_turbo "
    "if int(serp_data.get('query_param.need_certain_host', 0)) "
    "else 0.7 ** S.ungrouping_count_no_turbo"
)
FINE = "({}) * ({})".format(FINE_UNGROUPING, FINE_DUP)
FINE_AP = (
    "D.custom_formulas['fine_impl'] "
    "if (0.75 * D.custom_formulas['sinsig'] "
    "+ 0.1 * D.custom_formulas['dbd'] "
    "+ 0.18 * D.custom_formulas['kernel'] * D.custom_formulas['is_top_kernel'] "
    "+ 0.11 * D.custom_formulas['kernel'] * D.custom_formulas['is_bottom_kernel'] "
    "+ 0.05 * D.custom_formulas['ttrust'] "
    "+ 0.02 * D.custom_formulas['mobile_adaptivity']) "
    ">= 0 "
    "else 1.0"
)
FINE_AP_LIGHT = (
    "D.custom_formulas['fine_impl'] "
    "if (0.75 * D.custom_formulas['sinsig'] "
    "+ 0.18 * D.custom_formulas['kernel'] * D.custom_formulas['is_top_kernel'] "
    "+ 0.11 * D.custom_formulas['kernel'] * D.custom_formulas['is_bottom_kernel'] "
    "+ 0.05 * D.custom_formulas['ttrust'] "
    "+ 0.02 * D.custom_formulas['mobile_adaptivity']) "
    ">= 0 "
    "else 1.0"
)

TTRUST = "((1.0 * (S.ttrust_judgement if (S.ttrust_judgement + 0.1 > 1.0) else 3.0) - 2.0) / 3.0 - 0.66)"

base = ProximaPart(
    confidences=[
        {
            "name": "serp-failed",
            "threshold": 0.001,
            "condition": "SMALLER",
            "requirements": [],
            "regional": None,
            "yandex": False,
        },
        {
            "name": "empty-serp",
            "threshold": 0.005,
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

dup_req = ProximaPart(
    confidences=[
        {
            "name": "judged-dups-5",
            "threshold": 0.98,
            "condition": "GREATER",
            "requirements": [],
            "regional": None,
            "yandex": False,
        },
        {
            "name": "full-dups-missing-5",
            "threshold": 0.05,
            "condition": "SMALLER",
            "requirements": [
                "COMPONENT.judgements.full_duplicates"
            ],
            "regional": None,
            "yandex": False,
        },
    ],

    requirements={
        "COMPONENT.judgements.text_duplicates",
        "COMPONENT.judgements.full_duplicates",
    },
)

pure_dup = dup_req.copy()
pure_dup.label_script_parts = [FINE_DUP]

ungrouping_dcg = ProximaPart(
    requirements={
        "SERP.query_param.need_certain_host",
    },

    label_script_parts=[FINE_UNGROUPING],
)

div_position = ProximaPart(divisor="S.position")
maximum = ProximaPart(max_arg="-0.01")

pure_kernel = ProximaPart(
    confidences=[
        {
            "name": "proxima-business-kernel-2019-10-judged-5",
            "threshold": 0.92,
            "condition": "GREATER",
            "requirements": [],
            "regional": None,
            "yandex": False,
        },
    ],

    requirements={
        "COMPONENT.judgements.proxima_business_kernel_2019_10",
    },

    label_script_parts=["D.custom_formulas['kernel']"],

    custom_formulas={
        "kernel": "S.biz_kernel",
    },

    signals={"proxima_business_kernel_2019_10"},
)

pure_ttrust = ProximaPart(
    confidences=[
        {
            "name": "judged-ttrust-5",
            "threshold": 0.95,
            "condition": "GREATER",
            "requirements": [],
            "regional": None,
            "yandex": False,
        },
        {
            "name": "judged-ttrust-5",
            "threshold": 0.98,
            "condition": "GREATER",
            "requirements": [],
            "regional": None,
            "yandex": True,
        },
    ],

    requirements={
        "COMPONENT.judgements.ttrust_judgement",
    },

    label_script_parts=["D.custom_formulas['ttrust']"],

    custom_formulas={
        "ttrust": TTRUST,
    },

    signals={
        "ttrust_judgement",
    },

    scales={
    },
)

pure_sinsig = ProximaPart(
    confidences=[
        {
            "name": "judged-sinsig-kc-judgement-values-5",
            "threshold": 0.999,
            "condition": "GREATER",
            "requirements": [],
            "regional": None,
            "yandex": False,
        },
    ],

    requirements={
        "COMPONENT.judgements.sinsig_kc_judgement_values",
        "COMPONENT.judgements.sinsig_kc_slider_values",
    },

    label_script_parts=["D.custom_formulas['sinsig']"],

    custom_formulas={
        "sinsig": SINSIG,
    },

    signals={
        "sinsig_kc_judgement_values",
        "sinsig_kc_slider_values",
    },

    scales={
        "sinsig_kc_judgement_values": SINSIG_SCALE,
    },
)

pure_sinsig_no_turbo = ProximaPart(
    confidences=[
        {
            "name": "judged-sinsig-kc-no-turbo-judgement-values-5",
            "threshold": 0.999,
            "condition": "GREATER",
            "requirements": [],
            "regional": None,
            "yandex": False,
        },
    ],

    requirements={
        "COMPONENT.judgements.sinsig_kc_no_turbo_judgement_values",
        "COMPONENT.judgements.sinsig_kc_no_turbo_slider_values",
    },

    label_script_parts=["D.custom_formulas['sinsig']"],

    custom_formulas={
        "sinsig": SINSIG_NO_TURBO,
    },

    signals={
        "sinsig_kc_no_turbo_judgement_values",
        "sinsig_kc_no_turbo_slider_values",
    },

    scales={
        "sinsig_kc_no_turbo_judgement_values": SINSIG_SCALE,
    },
)

pure_dbd = ProximaPart(
    confidences=[
        {
            "name": "judged-dbd-eval-bt-score-5",
            "threshold": 0.995,
            "condition": "GREATER",
            "requirements": [],
            "regional": None,
            "yandex": False,
        },
    ],

    requirements={
        "COMPONENT.judgements.dbd_eval_bt_score",
    },

    label_script_parts=["D.custom_formulas['dbd']"],

    custom_formulas={
        "dbd": "S.dbd_eval_bt_score",
    },

    signals={"dbd_eval_bt_score"},

    scales={
    },
)

pure_dbd_no_turbo = ProximaPart(
    confidences=[
        {
            "name": "judged-dbd-eval-no-turbo-bt-score-5",
            "threshold": 0.995,
            "condition": "GREATER",
            "requirements": [],
            "regional": None,
            "yandex": False,
        },
    ],

    requirements={
        "COMPONENT.judgements.dbd_eval_no_turbo_bt_score",
    },

    label_script_parts=["D.custom_formulas['dbd']"],

    custom_formulas={
        "dbd": "S.dbd_eval_no_turbo_bt_score",
    },

    signals={"dbd_eval_no_turbo_bt_score"},

    scales={
    },
)

pure_mobile = ProximaPart(
    confidences=[
        {
            "name": "judged-mobile-access-5",
            "threshold": 0.98,
            "condition": "GREATER",
            "requirements": [
                "COMPONENT.judgements.five_cg_access"
            ],
            "regional": None,
            "yandex": False,
        },
    ],

    requirements={
        "COMPONENT.judgements.five_cg_access",
    },

    label_script_parts=["D.custom_formulas['mobile_adaptivity']"],

    custom_formulas={
        "mobile_adaptivity": "float(S.five_cg_access)",
    },

    signals={"five_cg_access"},

    scales={
        "five_cg_access": MOBILE_SCALE,
    },
)

pure_mobile_no_turbo = ProximaPart(
    confidences=[
        {
            "name": "judged-mobile-access-no-turbo-5",
            "threshold": 0.98,
            "condition": "GREATER",
            "requirements": [
                "COMPONENT.judgements.five_cg_access_no_turbo"
            ],
            "regional": None,
            "yandex": False,
        },
    ],

    requirements={
        "COMPONENT.judgements.five_cg_access_no_turbo",
    },

    label_script_parts=["D.custom_formulas['mobile_adaptivity']"],

    custom_formulas={
        "mobile_adaptivity": "float(S.five_cg_access_no_turbo)",
    },

    signals={"five_cg_access_no_turbo"},

    scales={
        "five_cg_access_no_turbo": MOBILE_SCALE,
    },
)

proxima_sinsig = ProximaPart(

    label_script_parts=["0.75 * D.custom_formulas['sinsig'] * D.custom_formulas['fine']"],

    custom_formulas={
        "sinsig": SINSIG,
    },

    signals={
        "sinsig_kc_judgement_values",
        "sinsig_kc_slider_values",
    },

    scales={
    },
)

proxima_sinsig_no_turbo = ProximaPart(

    label_script_parts=["0.75 * D.custom_formulas['sinsig'] * D.custom_formulas['fine']"],

    custom_formulas={
        "sinsig": SINSIG_NO_TURBO,
    },

    signals={
        "sinsig_kc_no_turbo_judgement_values",
        "sinsig_kc_no_turbo_slider_values",
    },

    scales={
    },
)

proxima_dbd = ProximaPart(
    confidences=[
        {
            "name": "judged-dbd-eval-bt-score-5",
            "threshold": 0.995,
            "condition": "GREATER",
            "requirements": [],
            "regional": None,
            "yandex": False,
        },
    ],

    requirements={
        "COMPONENT.judgements.dbd_eval_bt_score",
    },

    label_script_parts=["0.1 * D.custom_formulas['dbd'] * D.custom_formulas['fine']"],

    custom_formulas={
        "dbd": "S.dbd_eval_bt_score",
    },

    signals={
        "dbd_eval_bt_score",
    },

    scales={
    },
)

proxima_dbd_no_turbo = ProximaPart(
    confidences=[
        {
            "name": "judged-dbd-eval-no-turbo-bt-score-5",
            "threshold": 0.995,
            "condition": "GREATER",
            "requirements": [],
            "regional": None,
            "yandex": False,
        },
    ],

    requirements={
        "COMPONENT.judgements.dbd_eval_no_turbo_bt_score",
    },

    label_script_parts=["0.1 * D.custom_formulas['dbd'] * D.custom_formulas['fine']"],

    custom_formulas={
        "dbd": "S.dbd_eval_no_turbo_bt_score",
    },

    signals={
        "dbd_eval_no_turbo_bt_score",
    },

    scales={
    },
)

proxima_kernel = ProximaPart(
    confidences=[
        {
            "name": "proxima-business-kernel-2019-10-judged-5",
            "threshold": 0.92,
            "condition": "GREATER",
            "requirements": [],
            "regional": None,
            "yandex": False,
        },
    ],

    requirements={
        "COMPONENT.judgements.proxima_business_kernel_2019_10",
    },

    label_script_parts=[
        "0.18 * D.custom_formulas['kernel'] * D.custom_formulas['is_top_kernel'] * D.custom_formulas['fine']",
        "0.11 * D.custom_formulas['kernel'] * D.custom_formulas['is_bottom_kernel'] * D.custom_formulas['fine']",
    ],

    custom_formulas={
        "kernel": "(avg(L.sinsig_kc_judgement_values) > 0) * S.biz_kernel",
        "is_top_kernel": "serp_data.get('query_param.cluster') in {'5','7','8','10','18','30','31','33','34','35','36'}",
        "is_bottom_kernel": "serp_data.get('query_param.cluster') not in {'5','7','8','10','18','30','31','33','34','35','36'}",
    },

    signals={
        "proxima_business_kernel_2019_10",
    },

    scales={
    },
)

proxima_kernel_no_turbo = proxima_kernel.copy()
proxima_kernel_no_turbo.custom_formulas["kernel"] = (
    "(avg(L.sinsig_kc_no_turbo_judgement_values) > 0) * S.biz_kernel"
)

proxima_ttrust = ProximaPart(
    confidences=[
        {
            "name": "judged-ttrust-5",
            "threshold": 0.95,
            "condition": "GREATER",
            "requirements": [],
            "regional": None,
            "yandex": False,
        },
        {
            "name": "judged-ttrust-5",
            "threshold": 0.98,
            "condition": "GREATER",
            "requirements": [],
            "regional": None,
            "yandex": True,
        },
    ],

    requirements={
        "COMPONENT.judgements.ttrust_judgement",
    },

    label_script_parts=["0.05 * D.custom_formulas['ttrust']"],

    custom_formulas={
        "ttrust": "1.0 * (((" + SINSIG + ") > 0.3) * 0.8 + 0.2) * " + TTRUST,
    },

    signals={
        "ttrust_judgement",
    },

    scales={
    },
)

proxima_ttrust_no_turbo = proxima_ttrust.copy()
proxima_ttrust_no_turbo.custom_formulas["ttrust"] = "1.0 * (((" + SINSIG_NO_TURBO + ") > 0.3) * 0.8 + 0.2) * " + TTRUST

proxima_ttrust_no_turbo_wizard = proxima_ttrust_no_turbo.copy()
proxima_ttrust_no_turbo_wizard.confidences = [
    {
        "name": "ttrust-wizards-judged-5",
        "threshold": 0.95,
        "condition": "GREATER",
        "requirements": [],
        "regional": None,
        "yandex": False,
    },
    {
        "name": "ttrust-wizards-judged-5",
        "threshold": 0.98,
        "condition": "GREATER",
        "requirements": [],
        "regional": None,
        "yandex": True,
    },
]
# OFFLINESUP-443
# >>> ttrust_judgement = 5.0
# >>> ((1.0 * (ttrust_judgement if (ttrust_judgement + 0.1 > 1.0) else 3.0) - 2.0) / 3.0 - 0.66)
# 0.33999999999999997
proxima_ttrust_no_turbo_wizard.custom_formulas["ttrust"] = (
        "1.0 * ((("
        + SINSIG_NO_TURBO
        + ") > 0.3) * 0.8 + 0.2) * (0.34 if S.is_wizard else "
        + TTRUST
        + ")"
)

proxima_mobile = ProximaPart(
    confidences=[
        {
            "name": "judged-mobile-access-5",
            "threshold": 0.98,
            "condition": "GREATER",
            "requirements": [
                "COMPONENT.judgements.five_cg_access"
            ],
            "regional": None,
            "yandex": False,
        },
    ],

    requirements={
        "COMPONENT.judgements.five_cg_access",
    },

    label_script_parts=["0.02 * D.custom_formulas['mobile_adaptivity'] * D.custom_formulas['fine']"],

    custom_formulas={
        "mobile_adaptivity": "(avg(L.sinsig_kc_judgement_values) > 0) * float(S.five_cg_access)",
    },

    signals={"five_cg_access"},

    scales={
        "five_cg_access": MOBILE_SCALE,
    },
)

proxima_mobile_no_turbo = ProximaPart(
    confidences=[
        {
            "name": "judged-mobile-access-no-turbo-5",
            "threshold": 0.98,
            "condition": "GREATER",
            "requirements": [
                "COMPONENT.judgements.five_cg_access_no_turbo"
            ],
            "regional": None,
            "yandex": False,
        },
    ],

    requirements={
        "COMPONENT.judgements.five_cg_access_no_turbo",
    },

    label_script_parts=["0.02 * D.custom_formulas['mobile_adaptivity'] * D.custom_formulas['fine']"],

    custom_formulas={
        "mobile_adaptivity": "(avg(L.sinsig_kc_no_turbo_judgement_values) > 0) * float(S.five_cg_access_no_turbo)",
    },

    signals={"five_cg_access_no_turbo"},

    scales={
        "five_cg_access_no_turbo": MOBILE_SCALE,
    },
)

proxima_light_mobile_no_turbo = proxima_mobile_no_turbo.copy()
for c in proxima_light_mobile_no_turbo.confidences:
    if c["name"] == "judged-mobile-access-no-turbo-5":
        c["threshold"] = 0.94

proxima_base = ProximaPart(
    confidences=[
        {
            "name": "judged-sinsig-kc-judgement-values-5",
            "threshold": 0.999,
            "condition": "GREATER",
            "requirements": [],
            "regional": None,
            "yandex": False,
        },
    ],

    requirements={
        "COMPONENT.judgements.sinsig_kc_judgement_values",
        "COMPONENT.judgements.sinsig_kc_slider_values",
        "SERP.query_param.need_certain_host",
        "SERP.query_param.cluster",
    },

    custom_formulas={
        "fine_impl": FINE,
    },

    custom_formulas_ap={
        "fine": FINE_AP,
    },

    signals={
        "sinsig_kc_judgement_values",
        "sinsig_kc_slider_values",
    },

    scales={
        "sinsig_kc_judgement_values": SINSIG_SCALE,
    },
)
proxima_base.merge_with(base)
proxima_base.merge_with(dup_req)
proxima_base.merge_with(proxima_sinsig)
proxima_base.merge_with(proxima_dbd)
proxima_base.merge_with(proxima_kernel)
proxima_base.merge_with(proxima_ttrust)
proxima_base.merge_with(proxima_mobile)
proxima_base.label_script_parts = []

proxima_base_no_turbo = ProximaPart(
    confidences=[
        {
            "name": "judged-sinsig-kc-no-turbo-judgement-values-5",
            "threshold": 0.999,
            "condition": "GREATER",
            "requirements": [],
            "regional": None,
            "yandex": False,
        },
    ],

    requirements={
        "COMPONENT.judgements.sinsig_kc_no_turbo_judgement_values",
        "COMPONENT.judgements.sinsig_kc_no_turbo_slider_values",
        "SERP.query_param.need_certain_host",
        "SERP.query_param.cluster",
    },

    custom_formulas={
        "fine_impl": FINE,
    },

    custom_formulas_ap={
        "fine": FINE_AP,
    },

    signals={
        "sinsig_kc_no_turbo_judgement_values",
        "sinsig_kc_no_turbo_slider_values",
    },

    scales={
        "sinsig_kc_no_turbo_judgement_values": SINSIG_SCALE,
    },
)
proxima_base_light_wizard = proxima_base_no_turbo.copy()

proxima_base_no_turbo.merge_with(base)
proxima_base_no_turbo.merge_with(dup_req)
proxima_base_no_turbo.merge_with(proxima_sinsig_no_turbo)
proxima_base_no_turbo.merge_with(proxima_kernel_no_turbo)
proxima_base_no_turbo.merge_with(proxima_ttrust_no_turbo)
proxima_base_no_turbo.merge_with(proxima_mobile_no_turbo)

proxima_base_light = proxima_base_no_turbo.copy()
proxima_base_light.custom_formulas_ap["fine"] = FINE_AP_LIGHT
proxima_base_light.label_script_parts = []
for c in proxima_base_light.confidences:
    if c["name"] == "judged-mobile-access-no-turbo-5":
        c["threshold"] = 0.94

proxima_base_light_wizard.merge_with(base)
proxima_base_light_wizard.merge_with(dup_req)
proxima_base_light_wizard.merge_with(proxima_sinsig_no_turbo)
proxima_base_light_wizard.merge_with(proxima_kernel_no_turbo)
proxima_base_light_wizard.merge_with(proxima_ttrust_no_turbo_wizard)
proxima_base_light_wizard.merge_with(proxima_mobile_no_turbo)
proxima_base_light_wizard.custom_formulas_ap["fine"] = FINE_AP_LIGHT
proxima_base_light_wizard.label_script_parts = []

proxima_base_ba = proxima_base_no_turbo.copy()
proxima_base_ba.custom_formulas_ap["fine"] = FINE_AP_LIGHT
proxima_base_ba.scales["sinsig_kc_no_turbo_judgement_values"] = SINSIG_BA_SCALE
proxima_base_ba.label_script_parts = []

proxima_base_no_turbo.merge_with(proxima_dbd_no_turbo)
proxima_base_no_turbo.label_script_parts = []

proxima_sinsig_wizard_impact = ProximaPart(

    label_script_parts=["0.75 * D.custom_formulas['sinsig']"],

    custom_formulas={
        "sinsig": SINSIG_NO_TURBO,
    },

    signals={
        "sinsig_kc_no_turbo_judgement_values",
        "sinsig_kc_no_turbo_slider_values",
    },

    confidences=[
        {
            "name": "judged-sinsig-kc-no-turbo-judgement-values-5",
            "threshold": 0.995,
            "condition": "GREATER",
            "requirements": [],
            "regional": None,
            "yandex": False,
        },
    ],

    requirements={
        "COMPONENT.judgements.sinsig_kc_no_turbo_judgement_values",
        "COMPONENT.judgements.sinsig_kc_no_turbo_slider_values",
        "SERP.query_param.need_certain_host",
        "SERP.query_param.cluster",
    },

    scales={
        "sinsig_kc_no_turbo_judgement_values": SINSIG_SCALE,
    },
)

proxima_kernel_wizard_impact = proxima_kernel_no_turbo.copy()
proxima_kernel_wizard_impact.label_script_parts = [
    "0.18 * D.custom_formulas['kernel'] * D.custom_formulas['is_top_kernel']",
    "0.11 * D.custom_formulas['kernel'] * D.custom_formulas['is_bottom_kernel']",
]

proxima_ttrust_wizard_impact = proxima_ttrust_no_turbo_wizard.copy()

proxima_mobile_wizard_impact = proxima_mobile_no_turbo.copy()
proxima_mobile_wizard_impact.label_script_parts = ["0.02 * D.custom_formulas['mobile_adaptivity']"]

proxima_wizard_impact_base = ProximaPart.merge_list([
    base,
    proxima_sinsig_wizard_impact,
    proxima_kernel_wizard_impact,
    proxima_ttrust_wizard_impact,
    proxima_mobile_wizard_impact,
])

proxima_wizard_impact_value = proxima_wizard_impact_base.create_label_script()
proxima_wizard_impact_base.custom_formulas_ap["value"] = proxima_wizard_impact_value
proxima_wizard_impact_aggregate_script = (
    "sum(value/(pos+1) for pos, value in enumerate(all_custom_formulas['value']) if pos<5)"
    " - sum(value/(pos+1) for pos, value in enumerate(value for value, is_target in"
    " zip(all_custom_formulas['value'],all_custom_formulas['is_target_wizard']) if not is_target) if pos<5)"
)
proxima_wizard_impact_base.label_script_parts = ["D.custom_formulas['value']"]
