import blender_slices
from proxima_description import ProximaPart

# https://a.yandex-team.ru/arc/trunk/arcadia/quality/mstand_metrics/users/kulemyakin/offline_wizards/metrics_dicts.py
wiz_types_raw = {  # groups of different wizards
    "images": {5},
    "video": {19, 36, 100, 101, 102, 103},
    "geo": {11, 15, 81, 116},
    "adresa": {11, 81},
    "adresa_and_object": {11, 81, 89},
    "market": {8, 27},
    "other": {9, 12, 17, 18, 23, 24, 28, 29, 31, 41, 49, 64, 78, 84, 87, 112, 113, 117,
              118, 119, 127, 130},
    "music": {41},
    "yabs": {130},
    "object_answer": {89},
    "news": {7, 33},
    "no": set()
}
wiz_parts = {
    name: ProximaPart(
        custom_formulas={
            "is_target_wizard": "component.wizard_type in {!r}".format(sorted(types)),
        },
    ) for name, types in wiz_types_raw.items()
}

wiz_slices_parts = {
    name: ProximaPart(
        custom_formulas={
            "is_target_wizard": "1 if any(s in {!r} for s in component.json_slices) else 0".format(sorted(slices)),
        },
    ) for name, slices in blender_slices.blender_slices.items()
}
wiz_services_parts = {
    name: ProximaPart(
        custom_formulas={
            "is_target_wizard": "1 if get_yandex_service_by_url(component.get_scale('componentUrl', {{}}).get('pageUrl', '')) in {!r} else 0".format(service),
        },
    ) for name, service in blender_slices.yandex_services.items()
}
wiz_snippets_parts = {
    name: ProximaPart(
        custom_formulas={
            "is_target_wizard": "1 if any(s in {!r} for s in component.get_scale('json.SearchRuntimeInfo').get('Plugins', '').split('|')) else 0".format(snippets)
        }
    ) for name, snippets in blender_slices.blender_snippets.items()
}
wiz_snippets_parts_has_data = {
    name: ProximaPart(
        custom_formulas={
            "is_plugins": "1 if any(s in {!r} for s in component.get_scale('json.SearchRuntimeInfo').get('Plugins', '').split('|')) else 0".format(snippets),
            "is_filtered": "1 if any(s in {!r} for s in component.get_scale('json.SearchRuntimeInfo').get('FilteredP', '').split('|')) else 0".format(snippets),
            "is_disabled": "1 if any(s in {!r} for s in component.get_scale('json.SearchRuntimeInfo').get('DisabledP', '').split('|')) else 0".format(snippets)
        },
        custom_formulas_ap={
            "is_target_wizard": "max(D.custom_formulas['is_plugins'], D.custom_formulas['is_disabled'], D.custom_formulas['is_filtered'])"
        }
    ) for name, snippets in blender_slices.blender_snippets.items()
}

wiz_slices_parts["ALL_WIZARDS"] = ProximaPart(
    custom_formulas={"is_target_wizard": "1 if component.json_slices else 0"}
)
wiz_slices_parts["ALL_WIZARDS_EXCEPT_ENTITY_SEARCH"] = ProximaPart(
    custom_formulas={
        "is_target_wizard": '1 if component.json_slices and "ENTITY_SEARCH" not in component.json_slices else 0'}
)

slices_part = ProximaPart(
    requirements={
        "COMPONENT.json.slices",
    },
)
services_part = ProximaPart(
    requirements={
        "COMPONENT.componentUrl",
    },
)
