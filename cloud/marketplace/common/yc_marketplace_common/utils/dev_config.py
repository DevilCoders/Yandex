""" Build Development config """

import os


def dev_config(builtin_configs_dir_path):
    config_path = "./devel_config.yaml"
    config_template = "./devel_tmpl_config.yaml"
    with open(config_template, "r", encoding="utf-8") as template:
        # yaml can not parse utf8
        with open(config_path, "w", encoding="ascii") as cfg:
            for line in template:
                # Strip comments
                if line.strip().startswith("#"):
                    continue
                cfg.write(line.format(os.getenv("USER", "undef")).encode("ascii", "backslashreplace").decode("ascii"))

    return config_path
