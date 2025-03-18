def is_web_servicetype(action):
    return action.data.get("servicetype") in ("web", "touch", "pad")


def is_request(action):
    return action.data["type"] == "request"


def is_click(action):
    return action.data["type"] == "click"


def is_dynamic_click(action):
    return action.data["type"] == "dynamic-click"


def is_click_on_result_or_wizard(action):
    return action.data["restype"] in {"web", "wiz"}


def is_images_servicetype(action):
    return action.data.get("servicetype") == "images"


def is_img_click(action):
    return action.data["type"] == "images_img_open"
