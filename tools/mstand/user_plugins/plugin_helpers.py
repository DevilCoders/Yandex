import string
import yaqutils.six_helpers as usix

DEF_KWARGS_NAME = "default"

ALLOWED_CHARS_CUSTOM = " =,.-+_@|/$[](){}<>:"
ALLOWED_CHARS_READABLE = "A-Za-z0-9" + ALLOWED_CHARS_CUSTOM
ALLOWED_CHARS = set(string.ascii_letters + string.digits + ALLOWED_CHARS_CUSTOM)


def is_valid_name(name):
    """
    :type name: str
    :rtype: bool
    """
    # names are used as a keys in JSON dicts and in file names.
    # to avoid any problems, we allow only 'good' characters.
    return set(name) <= ALLOWED_CHARS


def replace_bad_chars(name):
    fixed_name = name
    replacements = {
        " ": "_",
        "$": ".DOLLAR.",
        "|": ".PIPE.",
        "/": ".SLASH.",
        "<": ".LESS.",
        ">": ".MORE.",
    }
    for char, replacement in usix.iteritems(replacements):
        fixed_name = fixed_name.replace(char, replacement)
    return fixed_name


def is_default_kwargs_name(kwargs_name):
    return kwargs_name == DEF_KWARGS_NAME or not kwargs_name
