from .colour import Color


def pallete(meta):
    result = {}
    for s in meta["groups"].keys():
        basic_color = meta["colors"][s]
        status_vals = meta["groups"][s]
        if len(status_vals) == 0:
            continue
        if len(status_vals) >= 1:
            light = Color(basic_color[0])
            dark = Color(basic_color[1])
            colors = list(light.range_to(dark, len(status_vals)))
            result.update(dict(zip(status_vals, colors)))
    return result


def schema_to_list(meta):
    groups = meta["groups"]
    result = []
    for g in groups.keys():
        result += groups[g]
    return result
