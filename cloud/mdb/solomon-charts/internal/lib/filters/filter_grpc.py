from .filter_signal_to_color import pallete


def __status_to_signalname(word: str) -> str:
    if word == "OK":
        return "OK"
    from . import snake_to_camel
    return snake_to_camel(word)


def status_to_signal(word: str, prefix: str = "", postfix: str = "") -> str:
    return "%s%s%s" % (prefix, __status_to_signalname(word), postfix)


def status_to_color(word, meta):
    return pallete(meta)[word].hex
