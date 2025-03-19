import os


def configure(name, config_bin, kikimr_bin, config_dir, template, defaults, init_flag=False, **kwargs):
    cfg_dir = "./" + name
    if init_flag:
        # FIXME (KIKIMR-5464): base name of configuration directory should be cfg for init scripts as it used for
        # relative paths for other files
        full_path = os.path.join(config_dir, name)
        if not os.path.isdir(os.path.join(config_dir, name)):
            os.mkdir(os.path.join(config_dir, name))
        os.symlink(name, os.path.join(config_dir, "cfg"))
        cfg_dir = "./cfg"
    cmd_args = [config_bin, "cfg", template, kikimr_bin, cfg_dir]

    for opt, optval in defaults.items():
        if isinstance(optval, bool) and not optval or optval is None:
            continue

        cmd_args.append("--" + opt.replace("_", "-"))
        if optval is True:
            continue

        cmd_args.append(str(optval))

    cmd = " ".join(cmd_args)
    cmd_ret = __states__['cmd.run'](cmd, cwd=config_dir, **kwargs)

    if init_flag:
        os.remove(os.path.join(config_dir, "cfg"))

        cmd_ret['comment'] += ' Config directory ./cfg was moved to ./%s' % name

    return cmd_ret
