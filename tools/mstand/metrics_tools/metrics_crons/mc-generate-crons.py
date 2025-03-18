import json

DEFAULT_FREQUENCES = (
    ("beak", "query-170972"),
    ("body", "query-170975"),
    ("tail", "query-170616"),
)

DEFAULT_DEVICES = (
    ("desktop", "query-170791"),
    ("touch", "query-170793"),
)

DEFAULT_COUNTRIES = (
    ("RU", "country-RU"),
    ("UA", "country-UA"),
    ("BY", "country-BY"),
    ("KZ", "country-KZ"),
    ("UZ", "country-UZ"),
    ("exUSSR", "country-AM-AZ-EE-GE-IL-KG-LT-LV-MD-TJ-TM"),
    ("xUSSR", "country-AM-AZ-EE-GE-IL-KG-LT-LV-MD-TJ-TM-UZ"),
    ("notRU", "country-AM-AZ-BY-EE-GE-IL-KG-KZ-LT-LV-MD-TJ-TM-UA-UZ")
)


def generate_spam(config):
    spam_devices = (
        ("desktop", "query-178887"),
        ("touch", "query-178889"),
    )
    spam_countries = (
        ("RU", "country-RU"),
        ("UA", "country-UA"),
        ("UZ", "country-UZ"),
        ("BY", "country-BY"),
        ("KZ", "country-KZ"),
        ("KUB", "country-KZ-UA-BY"),
        ("xUSSR", "country-AM-AZ-EE-GE-IL-KG-LT-LV-MD-TJ-TM-UZ"),
    )
    spam_aspect_filter = ["spam", "spam_mobile"]
    for d_name, d_query in spam_devices:
        config.append({
            "name": "+sample_spam.{}".format(d_name),
            "componentFilter": "onlySearchResult",
            "isVisible": True,
            "queryFilters": [d_query],
            "aspectFilter": spam_aspect_filter,
        })
        for c_name, c_query in spam_countries:
            config.append({
                "name": "+sample_spam.{}.{}".format(d_name, c_name),
                "componentFilter": "onlySearchResult",
                "isVisible": False,
                "queryFilters": [d_query, c_query],
                "aspectFilter": spam_aspect_filter,
            })


def generate_obj(config):
    obj_aspect_filter = ["object_answer"]

    for d_name, d_query in DEFAULT_DEVICES:
        config.append({
            "name": "all_{}".format(d_name),
            "componentFilter": "true",
            "isVisible": True,
            "queryFilters": [d_query],
            "aspectFilter": obj_aspect_filter,
        })

    obj_countries = (
        ("RU", "country-RU"),
        ("UA", "country-UA"),
        ("BY", "country-BY"),
        ("KZ", "country-KZ"),
        ("xUSSR", "country-AM-AZ-EE-GE-IL-KG-LT-LV-MD-TJ-TM-UZ"),
    )
    for c_name, c_query in obj_countries:
        config.append({
            "name": "all_{}".format(c_name),
            "componentFilter": "true",
            "isVisible": True,
            "queryFilters": [c_query],
            "aspectFilter": obj_aspect_filter,
        })
        for d_name, d_query in DEFAULT_DEVICES:
            config.append({
                "name": "all_{}_{}".format(c_name, d_name),
                "componentFilter": "true",
                "isVisible": False,
                "queryFilters": [c_query, d_query],
                "aspectFilter": obj_aspect_filter,
            })


def generate_main(config):
    default_aspect_filter = ["default"]

    config.extend([
        {
            "name": "all",
            "componentFilter": "true",
            "isVisible": True,
        },
        {
            "name": "nav",
            "componentFilter": "onlySearchResult",
            "isVisible": True,
            "queryFilters": ["db-987"],
            "aspectFilter": default_aspect_filter,
        },
        {
            "name": "dbd_init",
            "componentFilter": "onlySearchResult",
            "isVisible": True,
            "queryFilters": ["query-175905"],
            "aspectFilter": default_aspect_filter,
        },
        {
            "name": "sample_8000",
            "componentFilter": "onlySearchResult",
            "isVisible": True,
            "queryFilters": ["query-178906"],
            "aspectFilter": default_aspect_filter,
        },
        {
            "name": "sample_3000",
            "componentFilter": "onlySearchResult",
            "isVisible": True,
            "queryFilters": ["query-187091"],
            "aspectFilter": default_aspect_filter + ["competitor"],
        },
        {
            "name": "snipoffline_5000",
            "componentFilter": "onlySearchResult",
            "isVisible": True,
            "queryFilters": ["query-170614"],
            "aspectFilter": ["snipoffline"],
        },
        {
            "name": "porno",
            "componentFilter": "onlySearchResult",
            "isVisible": True,
            "queryFilters": ["query-188198"],
            "aspectFilter": ["default"],
        },
    ])

    for f_name, f_query in DEFAULT_FREQUENCES:
        config.append({
            "name": f_name,
            "componentFilter": "onlySearchResult",
            "isVisible": True,
            "queryFilters": [f_query],
            "aspectFilter": default_aspect_filter,
        })

    for d_name, d_query in DEFAULT_DEVICES:
        config.append({
            "name": d_name,
            "componentFilter": "onlySearchResult",
            "isVisible": True,
            "queryFilters": [d_query],
            "aspectFilter": default_aspect_filter,
        })

    for c_name, c_query in DEFAULT_COUNTRIES:
        if c_name in {"RU", "UA", "BY", "KZ", "UZ"}:
            aspect = default_aspect_filter + ["mstand"]
        else:
            aspect = default_aspect_filter
        config.append({
            "name": c_name,
            "componentFilter": "onlySearchResult",
            "isVisible": True,
            "queryFilters": [c_query],
            "aspectFilter": aspect,
        })

    kub_name = "KUB"
    kub_query = "country-KZ-UA-BY"
    config.append({
        "name": kub_name,
        "componentFilter": "onlySearchResult",
        "isVisible": True,
        "queryFilters": [kub_query],
        "aspectFilter": default_aspect_filter + ["mstand"],
    })
    for d_name, d_query in DEFAULT_DEVICES:
        config.append({
            "name": "{}_{}".format(kub_name, d_name),
            "componentFilter": "onlySearchResult",
            "isVisible": False,
            "queryFilters": [kub_query, d_query],
            "aspectFilter": default_aspect_filter,
        })


def generate_dashboard(config):
    dashboard_aspect_filter = ["dashboard"]

    for d_name, d_query in DEFAULT_DEVICES:
        for f_name, f_query in DEFAULT_FREQUENCES:
            config.append({
                "name": "{}_{}".format(d_name, f_name),
                "componentFilter": "onlySearchResult",
                "isVisible": False,
                "queryFilters": [d_query, f_query],
                "aspectFilter": dashboard_aspect_filter,
            })

    for c_name, c_query in DEFAULT_COUNTRIES:
        for f_name, f_query in DEFAULT_FREQUENCES:
            config.append({
                "name": "{}_{}".format(c_name, f_name),
                "componentFilter": "onlySearchResult",
                "isVisible": False,
                "queryFilters": [c_query, f_query],
                "aspectFilter": dashboard_aspect_filter,
            })
        for d_name, d_query in DEFAULT_DEVICES:
            config.append({
                "name": "{}_{}".format(c_name, d_name),
                "componentFilter": "onlySearchResult",
                "isVisible": False,
                "queryFilters": [c_query, d_query],
                "aspectFilter": dashboard_aspect_filter,
            })
            for f_name, f_query in DEFAULT_FREQUENCES:
                config.append({
                    "name": "{}_{}_{}".format(c_name, d_name, f_name),
                    "componentFilter": "onlySearchResult",
                    "isVisible": False,
                    "queryFilters": [c_query, d_query, f_query],
                    "aspectFilter": dashboard_aspect_filter,
                })


def sort_config(config):
    config.sort(key=lambda x: (0 if x.get("isVisible") else 1, x["name"].lower()))


def main():
    config = []
    generate_main(config)
    generate_dashboard(config)
    generate_obj(config)
    sort_config(config)
    with open("main_config.json", "w") as f:
        json.dump(config, f, indent=2, sort_keys=True, separators=(',', ': '))

    generate_spam(config)
    sort_config(config)
    with open("spam_config.json", "w") as f:
        json.dump(config, f, indent=2, sort_keys=True, separators=(',', ': '))


if __name__ == "__main__":
    main()
