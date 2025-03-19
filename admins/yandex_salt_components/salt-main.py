#!/usr/bin/python
import sys
import pathlib

SALT_MASTER = "salt-master"
SALT_MINION = "salt-minion"

SALT = "salt"
SALT_CALL = "salt-call"
SALT_CP = "salt-cp"
SALT_KEY = "salt-key"
SALT_RUN = "salt-run"
SALT_VENV = 'salt-venv'

SALT_ARC = 'salt-arc'


def main():
    binary_name = pathlib.Path(sys.argv[0]).name
    if binary_name == SALT_ARC:
        from admins.yandex_salt_components.misc.arc2salt import main

        main()
    else:
        from admins.yandex_salt_components.monkey_patching import patch_upsteam_salt

        patch_upsteam_salt()

        if binary_name == SALT:
            from salt.scripts import salt_main

            salt_main()
        elif binary_name == SALT_CALL:
            from salt.scripts import salt_call

            salt_call()
        elif binary_name == SALT_CP:
            from salt.scripts import salt_cp

            salt_cp()
        elif binary_name == SALT_KEY:
            from salt.scripts import salt_key

            salt_key()
        elif binary_name == SALT_RUN:
            from salt.scripts import salt_run

            salt_run()
        elif binary_name == SALT_MASTER:
            from salt.scripts import salt_master

            salt_master()
        elif binary_name == SALT_MINION:
            from salt.scripts import salt_minion
            from multiprocessing import freeze_support

            # windows support removed
            freeze_support()
            salt_minion()
        elif binary_name == SALT_VENV:
            # run custom python script in salt "virtualenv"
            import importlib.util

            script_path = pathlib.Path(sys.argv[1])
            sys.argv = sys.argv[1:]
            spec = importlib.util.spec_from_file_location("__main__", script_path)
            my_module = importlib.util.module_from_spec(spec)
            spec.loader.exec_module(my_module)
        else:
            print(f'Unknown {binary_name = }. Do nothing')
            exit(2)
