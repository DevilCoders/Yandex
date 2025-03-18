import re

RE_WWW = re.compile(r"^(https?://)?(www\.)?(.*)$")


def strip_protocol_www(url):
    ref_str = str(url).strip()
    m = RE_WWW.match(ref_str)
    return m.group(3)


RE_WEB = re.compile(r"^(yandex\.ru/yandsearch|ya\.ru/yandsearch|yandsearch\.yandex\.ru/yandsearch|m\.yandex\.ru/yandsearch).*")
RE_IMAGES = re.compile(r"^(images\.yandex\.ru|yandex\.ru/images|ya\.ru/images|m\.images\.yandex\.ru).*")
RE_VIDEO = re.compile(r"^(video\.yandex\.ru|yandex\.ru/video|ya\.ru/video|m\.video\.yandex\.ru).*")
RE_SERVICES = re.compile(r"^(yandex\.ru|ya\.ru|.+\.yandex\.ru|m\..+\.yandex\.ru).*")

RE_REFERER = (
    ("web", RE_WEB),
    ("images", RE_IMAGES),
    ("video", RE_VIDEO),
    ("services", RE_SERVICES),
)


def is_morda(request_value):
    return "msid=" in request_value


def get_referer_name(referer_value, request_value=""):
    if is_morda(request_value):
        return "morda"

    domain_with_path = strip_protocol_www(referer_value)

    for name, expression in RE_REFERER:
        if expression.match(domain_with_path):
            return name

    return "other"
