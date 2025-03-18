from jinja2.defaults import BLOCK_START_STRING
from jinja2.defaults import BLOCK_END_STRING
import re


def reproduce_tag(source, re_start, re_end=None):
    res = re.sub(re_start, r'{% raw %}\1{% endraw %}', source)
    if re_end:
        res = re.sub(re_end, r'{% raw %}\1{% endraw %}', res)
    return res


def reproduce_tag_0(tag, args=[], body=None):
    args = filter(None, args)  # Let's allow empty args, but remove them
    args_str = " " + " ".join(args) + " " if args else " "
    str = BLOCK_START_STRING + " " + tag + args_str + BLOCK_END_STRING
    if body is not None:
        str += body
        str += BLOCK_START_STRING + ' end' + tag + " " + BLOCK_END_STRING
    return str
