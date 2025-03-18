import click
import yaml
import json

import library.python.resource


def yaml_dump(obj):
    class NoAliasDumper(yaml.Dumper):
        def ignore_aliases(self, data):
            return True
    return yaml.dump(obj, Dumper=NoAliasDumper, width=2048)


@click.group()
def main():
    pass


def get_services():
    resource = library.python.resource.find("service_config.json")
    return set(section["service"] for section in json.loads(resource))


def generate_ruchkas_content():
    services = get_services()
    result = [
        {
            "id": "antirobot_stop_block_for_all",
            "path": "./stop_block_for_all",
            "name": "Stop block for all",
            "widget": "button",
            "validator": "choices_validator",
            "settings": {
                "choices": [
                    {
                        "key": "Stop block for all",
                        "value": "enable"
                    }
                ]
            }
        },
        {
            "id": "antirobot_stop_ban_for_all",
            "path": "./stop_ban_for_all",
            "name": "Stop ban for all",
            "widget": "button",
            "validator": "choices_validator",
            "settings": {
                "choices": [
                    {
                        "key": "Stop ban for all",
                        "value": "enable"
                    }
                ]
            }
        },
        {
            "id": "antirobot_shutdown",
            "path": "./shutdown",
            "name": "Shutdown",
            "widget": "button",
            "validator": "choices_validator",
            "settings": {
                "choices": [
                    {
                        "key": "shutdown",
                        "value": "enable"
                    }
                ]
            }
        },
        {
            "id": "antirobot_all_amnesty",
            "path": "./all_amnesty",
            "name": "Amnesty for all",
            "widget": "button",
            "validator": "choices_validator",
            "settings": {
                "choices": [
                    {
                        "key": "all_amnesty",
                        "value": "enable"
                    }
                ]
            }
        },
        {
            "id": "antirobot_stop_fury_for_all",
            "path": "./stop_fury_for_all",
            "name": "Stop fury requests",
            "widget": "button",
            "validator": "choices_validator",
            "settings": {
                "choices": [
                    {
                        "key": "Stop fury",
                        "value": "enable"
                    }
                ]
            }
        },
        {
            "id": "antirobot_stop_fury_preprod_for_all",
            "path": "./stop_fury_preprod_for_all",
            "name": "Stop preprod fury requests",
            "widget": "button",
            "validator": "choices_validator",
            "settings": {
                "choices": [
                    {
                        "key": "Stop preprod fury",
                        "value": "enable"
                    }
                ]
            }
        },
        {
            "id": "antirobot_disable_experiments",
            "path": "./disable_experiments",
            "name": "Disable all experiments",
            "widget": "button",
            "validator": "choices_validator",
            "settings": {
                "choices": [
                    {
                        "key": "Disable all experiments",
                        "value": "enable"
                    }
                ]
            }
        },
        {
            "id": "antirobot_stop_yql_for_all",
            "path": "./stop_yql_for_all",
            "name": "Stop YQL rules for all",
            "widget": "button",
            "validator": "choices_validator",
            "settings": {
                "choices": [
                    {
                        "key": "Stop YQL rules for all",
                        "value": "enable"
                    }
                ]
            }
        },
        {
            "id": "antirobot_500_enable",
            "path": "./500_enable",
            "name": "Enable 500 errors for all",
            "widget": "button",
            "validator": "choices_validator",
            "settings": {
                "choices": [
                    {
                        "key": "Enable 500 errors for all",
                        "value": "enable"
                    }
                ]
            }
        },
        {
            "id": "antirobot_disable_experiments",
            "path": "./disable_experiments",
            "name": "Disable antirobot experiments",
            "widget": "button",
            "validator": "choices_validator",
            "settings": {
                "choices": [
                    {
                        "key": "Disable antirobot experiments",
                        "value": "enable"
                    }
                ]
            }
        },
        {
            "id": "antirobot_delete_blocks_dump",
            "path": "./delete_blocks_dump",
            "name": "delete blocks_dump at startup",
            "widget": "button",
            "validator": "choices_validator",
            "settings": {
                "choices": [
                    {
                        "key": "Delete blocks_dump at startup",
                        "value": "enable"
                    }
                ]
            }
        },
        {
            "id": "antirobot_delete_robot_uids_dump",
            "path": "./delete_robot_uids_dump",
            "name": "delete robot_uids_dump at startup",
            "widget": "button",
            "validator": "choices_validator",
            "settings": {
                "choices": [
                    {
                        "key": "Delete robot_uids_dump at startup",
                        "value": "enable"
                    }
                ]
            }
        },
        {
            "id": "antirobot_delete_search_engine_bots",
            "path": "./delete_search_engine_bots",
            "name": "delete search_engine_bots at startup",
            "widget": "button",
            "validator": "choices_validator",
            "settings": {
                "choices": [
                    {
                        "key": "Delete search_engine_bots at startup",
                        "value": "enable"
                    }
                ]
            }
        },
        {
            "id": "antirobot_delete_userbase",
            "path": "./delete_userbase",
            "name": "delete userbase at startup",
            "widget": "button",
            "validator": "choices_validator",
            "settings": {
                "choices": [
                    {
                        "key": "Delete userbase at startup",
                        "value": "enable"
                    }
                ]
            }
        },
        {
            "id": "antirobot_delete_cbb_cache",
            "path": "./delete_cbb_cache",
            "name": "delete cbb_cache at startup",
            "widget": "button",
            "validator": "choices_validator",
            "settings": {
                "choices": [
                    {
                        "key": "Delete cbb_cache at startup",
                        "value": "enable"
                    }
                ]
            }
        },
        {
            "id": "antirobot_stop_discovery_for_all",
            "path": "./stop_discovery_for_all",
            "name": "Stop YP discovery for all",
            "widget": "button",
            "validator": "choices_validator",
            "settings": {
                "choices": [
                    {
                        "key": "Stop YP discovery for all",
                        "value": "enable"
                    }
                ]
            }
        },
        {
            "id": "antirobot_disable_catboost_whitelist_all",
            "path": "./disable_catboost_whitelist_all",
            "name": "disable catboost whitelist for al",
            "widget": "button",
            "validator": "choices_validator",
            "settings": {
                "choices": [
                    {
                        "key": "Disable catboost whitelist for all",
                        "value": "enable"
                    }
                ]
            }
        },
    ]

    for service in services:
        main_ban_section = {
            "id": f"antirobot_allow_{service}_main_ban",
            "path": "./allow_main_ban",
            "name": "Allow main ban",
            "widget": "button",
            "validator": "choices_validator",
            "settings": {
                "choices": [
                    {
                        "key": "Allow main ban",
                        "value": service
                    }
                ]
            }
        }
        if service == "yaru":
            main_ban_section["is_permanent"] = True

        if service in ["web", "yaru", "morda"]:
            result.append({
                "id": f"antirobot_allow_{service}_dzensearch_ban",
                "path": "./allow_dzensearch_ban",
                "name": "Allow dzensearch ban",
                "widget": "button",
                "validator": "choices_validator",
                "settings": {
                    "choices": [
                        {
                            "key": "Allow main ban",
                            "value": service
                        }
                    ]
                }
            })

        if service == "market":
            result.append({
                "id": f"antirobot_dummy_{service}",
                "path": "./dummy",
                "name": "Dummy flag" + (" for other" if service == "other" else ""),
                "widget": "button",
                "validator": "choices_validator",
                "settings": {
                    "choices": [
                        {
                            "key": "Dummy flag" + (" for other" if service == "other" else ""),
                            "value": service
                        }
                    ]
                }
            })

        result.append({
            "id": f"antirobot_suspicious_ban_for_{service}",
            "path": "./suspicious_ban",
            "name": "Suspicious ban" + (" for other" if service == "other" else ""),
            "widget": "button",
            "validator": "choices_validator",
            "settings": {
                "choices": [
                    {
                        "key": "Suspicious ban" + (" for other" if service == "other" else ""),
                        "value": service
                    }
                ]
            }
        })

        result.append({
            "id": f"antirobot_suspicious_block_for_{service}",
            "path": "./suspicious_block",
            "name": "Suspicious block" + (" for other" if service == "other" else ""),
            "widget": "button",
            "validator": "choices_validator",
            "settings": {
                "choices": [
                    {
                        "key": "Suspicious block" + (" for other" if service == "other" else ""),
                        "value": service
                    }
                ]
            }
        })

        result.append({
            "id": f"antirobot_suspicious_429_for_{service}",
            "path": "./suspicious_429",
            "name": "Suspicious 429 on desktop" + (" for other" if service == "other" else ""),
            "widget": "button",
            "validator": "choices_validator",
            "settings": {
                "choices": [
                    {
                        "key": "Suspicious 429 on desktop" + (" for other" if service == "other" else ""),
                        "value": service
                    }
                ]
            }
        })

        result.append({
            "id": f"antirobot_suspicious_mobile_429_for_{service}",
            "path": "./suspicious_mobile_429",
            "name": "Suspicious 429 on mobile" + (" for other" if service == "other" else ""),
            "widget": "button",
            "validator": "choices_validator",
            "settings": {
                "choices": [
                    {
                        "key": "Suspicious 429 on mobile" + (" for other" if service == "other" else ""),
                        "value": service
                    }
                ]
            }
        })

        result.append(main_ban_section)

        result.append({
            "id": f"antirobot_stop_ban_for_{service}",
            "path": "./stop_ban",
            "name": "Stop ban" + (" for other" if service == "other" else ""),
            "widget": "button",
            "validator": "choices_validator",
            "settings": {
                "choices": [
                    {
                        "key": "Stop ban" + (" for other" if service == "other" else ""),
                        "value": service
                    }
                ]
            }
        })
        result.append({
            "id": f"antirobot_{service}_allow_ban_all",
            "path": "./allow_ban_all",
            "name": "Allow Antirobot to ban for all requests",
            "widget": "button",
            "validator": "choices_validator",
            "settings": {
                "choices": [
                    {
                        "key": "Allow Antirobot to ban for all requests",
                        "value": service
                    }
                ]
            }
        })
        result.append({
            "id": f"antirobot_{service}_allow_show_captcha_all",
            "path": "./allow_show_captcha_all",
            "name": "Allow Antirobot to show captcha to all requests",
            "widget": "button",
            "validator": "choices_validator",
            "settings": {
                "choices": [
                    {
                        "key": "Allow Antirobot to show captcha to all requests",
                        "value": service
                    }
                ]
            }
        })
        result.append({
            "id": f"antirobot_stop_block_for_{service}",
            "path": "./stop_block",
            "name": "Stop block",
            "widget": "button",
            "validator": "choices_validator",
            "settings": {
                "choices": [
                    {
                        "key": "Stop block",
                        "value": service
                    }
                ]
            }
        })
        result.append({
            "id": f"antirobot_disable_preview_ident_type_for_{service}",
            "path": "./preview_ident_type_enabled",
            "name": "Disable preview ident type",
            "widget": "button",
            "validator": "choices_validator",
            "settings": {
                "choices": [
                    {
                        "key": "Disable preview ident type",
                        "value": service
                    }
                ]
            }
        })
        result.append({
            "id": f"antirobot_cbb_panic_mode_for_{service}",
            "path": "./cbb_panic_mode",
            "name": "Enable CBB panic mode",
            "widget": "button",
            "validator": "choices_validator",
            "settings": {
                "choices": [
                    {
                        "key": "Enable CBB panic mode",
                        "value": service
                    }
                ]
            }
        })
        result.append({
            "id": f"antirobot_stop_yql_for_{service}",
            "path": "./stop_yql",
            "name": "Stop YQL rules",
            "widget": "button",
            "validator": "choices_validator",
            "settings": {
                "choices": [
                    {
                        "key": "Stop YQL rules",
                        "value": service
                    }
                ]
            }
        })
        result.append({
            "id": f"antirobot_500_disable_for_{service}",
            "path": "./500_disable_service",
            "name": "Disable 500 errors",
            "widget": "button",
            "validator": "choices_validator",
            "settings": {
                "choices": [
                    {
                        "key": "Disable 500 errors",
                        "value": service
                    }
                ]
            }
        })
        result.append({
            "id": f"antirobot_{service}_amnesty",
            "path": "./amnesty",
            "name": "Amnesty",
            "widget": "button",
            "validator": "choices_validator",
            "settings": {
                "choices": [
                    {
                        "key": "Ask Antirobot to give amnesty to robots",
                        "value": service
                    }
                ]
            }
        })
        result.append({
            "id": f"antirobot_disable_catboost_whitelist_{service}",
            "path": "./disable_catboost_whitelist",
            "name": "Disable catboost whitelist",
            "widget": "button",
            "validator": "choices_validator",
            "settings": {
                "choices": [
                    {
                        "key": "Antirobot disable catboost whitelist",
                        "value": service
                    }
                ]
            }
        })

    return result


def generate_locations_content():
    services = get_services()
    responsible = [
        "toshchakov",
        "ashagarov",
    ]
    all_section_ruchkas = [
        "antirobot_stop_block_for_all",
        "antirobot_stop_ban_for_all",
        "antirobot_all_amnesty",
        "antirobot_shutdown",
        "antirobot_stop_fury_for_all",
        "antirobot_stop_fury_preprod_for_all",
        "antirobot_disable_experiments",
        "antirobot_stop_yql_for_all",
        "antirobot_500_enable",
        "antirobot_delete_blocks_dump",
        "antirobot_delete_robot_uids_dump",
        "antirobot_delete_search_engine_bots",
        "antirobot_delete_userbase",
        "antirobot_delete_cbb_cache",
        "antirobot_stop_discovery_for_all",
        "antirobot_disable_catboost_whitelist_all",
    ]
    result = {
        "_all": {
            "groups": {
                "prestable": {
                    "filter": "I@a_itype_antirobot . I@a_ctype_prestable . [I@a_geo_sas I@a_geo_vla I@a_geo_man]",
                    "ruchkas": all_section_ruchkas,
                    "responsible": responsible
                },
                "stable_man": {
                    "filter": "I@a_itype_antirobot . I@a_ctype_prod . I@a_geo_man",
                    "ruchkas": all_section_ruchkas,
                    "responsible": responsible
                },
                "stable_sas": {
                    "filter": "I@a_itype_antirobot . I@a_ctype_prod . I@a_geo_sas",
                    "ruchkas": all_section_ruchkas,
                    "responsible": responsible
                },
                "stable_vla": {
                    "filter": "I@a_itype_antirobot . I@a_ctype_prod . I@a_geo_vla",
                    "ruchkas": all_section_ruchkas,
                    "responsible": responsible
                },
            }
        }
    }
    for service in services:
        ruchkas = [
            f"antirobot_stop_block_for_{service}",
            f"antirobot_stop_ban_for_{service}",
            f"antirobot_allow_{service}_main_ban",
            f"antirobot_{service}_allow_ban_all",
            f"antirobot_{service}_allow_show_captcha_all",
            f"antirobot_{service}_amnesty",
            f"antirobot_disable_preview_ident_type_for_{service}",
            f"antirobot_cbb_panic_mode_for_{service}",
            f"antirobot_stop_yql_for_{service}",
            f"antirobot_500_disable_for_{service}",
            f"antirobot_disable_catboost_whitelist_{service}",
            f"antirobot_suspicious_ban_for_{service}",
            f"antirobot_suspicious_block_for_{service}",
            f"antirobot_suspicious_429_for_{service}",
            f"antirobot_suspicious_mobile_429_for_{service}",
        ]

        if service in ["web", "yaru", "morda"]:
            ruchkas.extend([
                f"antirobot_allow_{service}_dzensearch_ban",
            ])

        if service == "market":
            ruchkas.extend([
                f"antirobot_dummy_{service}",
            ])

        result[service] = {
            "groups": {
                "prestable": {
                    "filter": "I@a_itype_antirobot . I@a_ctype_prestable . [I@a_geo_sas I@a_geo_vla I@a_geo_man]",
                    "ruchkas": ruchkas,
                    "responsible": responsible,
                },
                "stable": {
                    "filter": "I@a_itype_antirobot . I@a_ctype_prod . [I@a_geo_sas I@a_geo_vla I@a_geo_man]",
                    "ruchkas": ruchkas,
                    "responsible": responsible,
                },
            }
        }

    ruchkas = set(section["id"] for section in generate_ruchkas_content())
    for section in result.values():
        for group in section["groups"].values():
            for ruchka in group["ruchkas"]:
                assert ruchka in ruchkas, f"ruchka '{ruchka}' is not present in ruchkas list"

    return result


@main.command()
@click.option("--prev-config-file", type=str)  # https://nanny.yandex-team.ru/ui/#/its/config/edit/ruchkas/
@click.option("--result-config-file", type=str, default="ruchkas.yml")
def ruchkas(prev_config_file, result_config_file):
    prev_config = {}
    if prev_config_file:
        with open(prev_config_file) as inp:
            prev_config = yaml.load(inp, Loader=yaml.FullLoader)

    ruchkas_content = generate_ruchkas_content()

    with open(result_config_file, "w") as out:
        out.write(yaml_dump({"content": ruchkas_content}))

    extra_ids = set(r["id"] for r in prev_config["content"] if "antirobot" in r["id"]) -\
                {"balancer_antirobot_module_switch"}

    new_sections = []
    changed_sections_prev = []
    changed_sections_new = []

    for ruchka_section in ruchkas_content:
        prev_section = None
        for r in prev_config["content"]:
            if r["id"] == ruchka_section["id"]:
                prev_section = r
                break
        to_str = lambda x: json.dumps(x, indent=4, sort_keys=True)
        if prev_section is None:
            new_sections.append(ruchka_section)
        elif to_str(prev_section) != to_str(ruchka_section):
            changed_sections_prev.append(prev_section)
            changed_sections_new.append(ruchka_section)

        if ruchka_section['id'] in extra_ids:
            extra_ids.remove(ruchka_section['id'])

    print("\n=== New sections ===\n")
    print(yaml_dump({"content": new_sections}))

    print("\n=== Changed sections (delete) ===\n")
    print(yaml_dump({"content": changed_sections_prev}))

    print("\n=== Changed sections (add) ===\n")
    print(yaml_dump({"content": changed_sections_new}))

    for section in prev_config["content"]:
        if section["id"] in extra_ids:
            print("Extra section found:")
            print(json.dumps(section, indent=4))
    assert len(extra_ids) == 0, f"Extra ids found in previous config: {', '.join(extra_ids)}"


@main.command()
@click.option("--prev-config-file", type=str)  # https://nanny.yandex-team.ru/ui/#/its/config/edit/locations/
@click.option("--result-config-file", type=str, default="locations.yml")
def locations(prev_config_file, result_config_file):
    prev_config = {}
    if prev_config_file:
        with open(prev_config_file) as inp:
            prev_config = yaml.load(inp, Loader=yaml.FullLoader)["content"]["groups"]["antirobot"]["groups"]["antirobot"]["groups"]

    locations_content = generate_locations_content()

    with open(result_config_file, "w") as out:
        out.write(yaml_dump({"groups": locations_content}))

    extra_ids = set(prev_config.keys())

    new_sections = {}
    changed_sections_prev = {}
    changed_sections_new = {}

    for id, location_section in locations_content.items():
        prev_section = prev_config.get(id)
        to_str = lambda x: json.dumps(x, indent=4, sort_keys=True)
        if prev_section is None:
            new_sections[id] = location_section
        elif to_str(prev_section) != to_str(location_section):
            changed_sections_prev[id] = prev_section
            changed_sections_new[id] = location_section

        if id in extra_ids:
            extra_ids.remove(id)

    print("\n=== New sections ===\n")
    print(yaml_dump({"content": {"groups": {"antirobot": {"groups": new_sections}}}}))

    print("\n=== Changed sections (delete) ===\n")
    print(yaml_dump({"content": {"groups": {"antirobot": {"groups": changed_sections_prev}}}}))

    print("\n=== Changed sections (add) ===\n")
    print(yaml_dump({"content": {"groups": {"antirobot": {"groups": changed_sections_new}}}}))

    for id, section in prev_config.items():
        if id in extra_ids:
            print(f"Extra section '{id}' found:")
            print(json.dumps(section, indent=4))
    assert len(extra_ids) == 0, f"Extra ids found in previous config: {', '.join(extra_ids)}"
