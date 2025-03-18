from mstand_enums.mstand_online_enums import ServiceEnum

ABT_DESKTOP_PLATFORMS = ["desktop"]
ABT_TOUCH_PLATFORMS = ["touch", "tablet"]
ABT_PLATFORMS = ABT_DESKTOP_PLATFORMS + ABT_TOUCH_PLATFORMS

MSTAND_TO_ABT_RULES = {
    ServiceEnum.WEB_DESKTOP: (ABT_DESKTOP_PLATFORMS, ["web"]),
    ServiceEnum.WEB_DESKTOP_EXTENDED: (ABT_DESKTOP_PLATFORMS, ["web"]),
    ServiceEnum.WEB_TOUCH: (ABT_TOUCH_PLATFORMS, ["web", "touch", "padsearch"]),
    ServiceEnum.WEB_TOUCH_EXTENDED: (ABT_TOUCH_PLATFORMS, ["web", "touch", "padsearch"]),

    ServiceEnum.IMAGES: (ABT_PLATFORMS, ["images"]),
    ServiceEnum.INTRASEARCH: (ABT_PLATFORMS, ["intrasearch"]),
    ServiceEnum.MARKET_SESSIONS_STAT: (ABT_PLATFORMS, ["market"]),
    ServiceEnum.MORDA: (ABT_PLATFORMS, ["morda"]),
    ServiceEnum.NEWS: (ABT_PLATFORMS, ["news"]),
    ServiceEnum.TOLOKA: (ABT_PLATFORMS, ["toloka"]),
    ServiceEnum.VIDEO: (ABT_PLATFORMS, ["video"]),
}
