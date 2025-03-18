# Пример запуска:
# ./render_cpp_incs . ~/arcadia/antirobot/captcha ru tr && cat ru.cpp.inc tr.cpp.inc

import os
import sys

from antirobot.scripts.captcha_localize.render import render_html


if __name__ == "__main__":
    out_dir = sys.argv[1]
    templ_dir = sys.argv[2]
    cpp_template = "cpp.tpl"
    lang_srcs = sys.argv[3:]

    if not os.path.exists(out_dir):
        os.makedirs(out_dir)

    for lang in lang_srcs:
        templ_to_render = os.path.join("lang", lang)
        out_file = os.path.join(out_dir, lang + ".cpp.inc")
        render_html(out_file, templ_dir, templ_to_render, cpp_template)
