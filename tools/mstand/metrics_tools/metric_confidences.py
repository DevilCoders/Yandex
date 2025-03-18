from typing import Dict
from typing import List


def get_serp_confidences() -> List[Dict]:
    return [
        {
            "condition": "SMALLER",
            "name": "empty-serp",
            "regional": None,
            "requirements": [],
            "threshold": 0.01,
            "yandex": False
        },
        {
            "condition": "SMALLER",
            "name": "empty-serp",
            "regional": None,
            "requirements": [],
            "threshold": 0.001,
            "yandex": True
        },
        {
            "condition": "SMALLER",
            "name": "serp-failed",
            "regional": None,
            "requirements": [],
            "threshold": 0.003,
            "yandex": False,
        },
    ]


def get_look_confidences(depth: int, look_enrichment_key: str) -> List[Dict]:
    if depth == 30 or depth == -1:
        threshold = 0.4
        depth_name = "30"
    elif depth == 5 or depth == 4 or depth == 1 or depth == 2:
        threshold = 0.995
        depth_name = "5"
    else:
        raise Exception(f"depth = {depth} in not implemented")
    return [
        {
            "condition": "GREATER",
            "name": f"judged-{look_enrichment_key}-eval-for-uno-{depth_name}",
            "regional": None,
            "requirements": [],
            "threshold": threshold,
            "yandex": False,
        }
    ]


def get_sinsig_confidences(depth: int) -> List[Dict]:
    if depth == 30 or depth == -1:
        return [
            {
                "name": "judged-sinsig-kc-no-turbo-for-uno-30",
                "threshold": 0.8,
                "condition": "GREATER",
                "requirements": [],
                "regional": None,
                "yandex": True
            },
            {
                "name": "judged-sinsig-kc-no-turbo-for-uno-30",
                "threshold": 0.75,
                "condition": "GREATER",
                "requirements": [],
                "regional": None,
                "yandex": False
            },
        ]
    elif depth == 5 or depth == 4:
        return [
            {
                "name": "judged-sinsig-kc-no-turbo-for-uno-5",
                "threshold": 0.99,
                "condition": "GREATER",
                "requirements": [],
                "regional": None,
                "yandex": False
            },
        ]
    else:
        raise Exception(f"depth = {depth} in not implemented")


def get_proxima_2020_light_confidences(depth: int) -> List[Dict]:
    confidences = get_sinsig_confidences(depth)
    if depth == 30 or depth == -1:
        depth_name = "30"
        plag_fine_threshold = 0.4
        comb_host_threshold = 0.4
    elif depth == 5 or depth == 4:
        depth_name = "5"
        plag_fine_threshold = 0.75
        comb_host_threshold = 0.85
    else:
        raise Exception(f"depth = {depth} in not implemented")
    confidences.extend([
        {
            "name": f"judged-comb-host-2021-for-uno-{depth_name}",
            "threshold": comb_host_threshold,
            "condition": "GREATER",
            "requirements": [],
            "regional": None,
            "yandex": False
        },
        {
            "name": f"judged-plag-fine-2021-for-uno-{depth_name}",
            "threshold": plag_fine_threshold,
            "condition": "GREATER",
            "requirements": [],
            "regional": None,
            "yandex": False
        },
        {
            "name": f"judged-dup-fine-2021-for-uno-{depth_name}",
            "threshold": plag_fine_threshold,
            "condition": "GREATER",
            "requirements": [],
            "regional": None,
            "yandex": False
        },
        {
            "name": f"judged-super-dup-only-fine-2021-for-uno-{depth_name}",
            "threshold": plag_fine_threshold,
            "condition": "GREATER",
            "requirements": [],
            "regional": None,
            "yandex": False
        },
        {
            "name": f"judged-super-text-dup-fine-2021-for-uno-{depth_name}",
            "threshold": plag_fine_threshold,
            "condition": "GREATER",
            "requirements": [],
            "regional": None,
            "yandex": False
        }
    ])
    return confidences


def get_defect_rate_confidences():
    confidences = get_serp_confidences()
    confidences.append({
        "condition": "GREATER",
        "name": f"judged-serp-irrel-10",
        "regional": None,
        "requirements": [],
        "threshold": 0.99,
        "yandex": False,
    })
