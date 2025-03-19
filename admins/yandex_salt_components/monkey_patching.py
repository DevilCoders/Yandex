import sys


def get_salt_module_path() -> str:
    return "/opt/yandex-salt-components"


def reimport_fileserver():
    from admins.yandex_salt_components import fileserver
    from admins.yandex_salt_components.fileserver import dynamic_roots

    sys.modules["fileserver"] = fileserver
    sys.modules["fileserver.dynamic_roots"] = dynamic_roots


def reimport_pillar():
    from admins.yandex_salt_components import pillar
    from admins.yandex_salt_components.pillar import dynamic_roots

    sys.modules["pillar"] = pillar
    sys.modules["pillar.dynamic_roots"] = dynamic_roots


def reimport_grains():
    from admins.yandex_salt_components import grains
    from admins.yandex_salt_components.grains import cauth
    from admins.yandex_salt_components.grains import conductor
    from admins.yandex_salt_components.grains import invapi
    from admins.yandex_salt_components.grains import yandex_common

    sys.modules["grains"] = grains
    sys.modules["grains.cauth"] = cauth
    sys.modules["grains.conductor"] = conductor
    sys.modules["grains.invapi"] = invapi
    sys.modules["grains.yandex_common"] = yandex_common


def reimport_modules():
    from admins.yandex_salt_components import modules
    from admins.yandex_salt_components.modules import ldmerge
    from admins.yandex_salt_components.modules import conductor
    from admins.yandex_salt_components.modules import yav
    from admins.yandex_salt_components.grains import invapi

    sys.modules["modules"] = modules
    sys.modules["modules.ldmerge"] = ldmerge
    sys.modules["modules.conductor"] = conductor
    sys.modules["modules.invapi"] = invapi
    sys.modules["modules.yav"] = yav


def reimport_states():
    from admins.yandex_salt_components import states
    from admins.yandex_salt_components.states import monrun
    from admins.yandex_salt_components.states import yafile

    sys.modules["states"] = states
    sys.modules["states.monrun"] = monrun
    sys.modules["states.yafile"] = yafile


def monkeypath_salt_lazy_loader():
    """
    states/yanetconfig.py # нужен python-netconfig в аркадии сейчас стейт поломан
    """

    # allow to load external salt modules
    sys.path.append(get_salt_module_path())
    from salt.loader import lazy

    old_refresh = lazy.LazyLoader._refresh_file_mapping

    def _refresh_file_mapping(self):
        old_refresh(self)
        if self.tag == "grains":
            blacklist = set(self.opts.get("arcadia_builtin_grains_blacklist", []))
            whitelist = set(self.opts.get("arcadia_builtin_extra_grains_whitelist", ['cauth', 'conductor', 'yandex_common']))
            for mod_name in ('cauth', 'conductor', 'yandex_common', 'invapi'):
                if mod_name not in blacklist and mod_name in whitelist:
                    self.file_mapping[mod_name] = (f"grains.{mod_name}", ".o", 0)
        elif self.tag == "module":
            self.file_mapping["ldmerge"] = ("modules.ldmerge", ".o", 0)
            self.file_mapping["conductor"] = ("modules.conductor", ".o", 0)
            self.file_mapping["yav"] = ("modules.yav", ".o", 0)
            self.file_mapping["invapi"] = ("modules.invapi", ".o", 0)
        elif self.tag == "states":
            self.file_mapping["monrun"] = ("states.monrun", ".o", 0)
            self.file_mapping["yafile"] = ("states.yafile", ".o", 0)
        elif self.tag == "fileserver":
            self.file_mapping["dynamic_roots"] = ("fileserver.dynamic_roots", ".o", 0)
        elif self.tag == "pillar":
            self.file_mapping["dynamic_roots"] = ("pillar.dynamic_roots", ".o", 0)

    lazy.LazyLoader._refresh_file_mapping = _refresh_file_mapping


def patch_upsteam_salt():
    # Adding the trace method to the logger as a side effect
    # Therefore, the logger must be imported strictly after this call
    assert "logging" not in sys.modules
    monkeypath_salt_lazy_loader()

    reimport_fileserver()
    reimport_pillar()
    reimport_grains()
    reimport_modules()
    reimport_states()
