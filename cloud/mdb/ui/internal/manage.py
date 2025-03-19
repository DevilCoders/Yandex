#!/usr/bin/env python
import os
import sys

# print(sys.path)
# sys.path.append('./mdbui')

if __name__ == "__main__":
    os.environ.setdefault("DJANGO_SETTINGS_MODULE", "cloud.mdb.ui.internal.mdbui.settings")
    os.environ.setdefault("PGSSLROOTCERT", "~/.certs/allCAs.pem")
    try:
        from django.core.management import execute_from_command_line
    except ImportError as exc:
        raise ImportError(
            "Couldn't import Django. Are you sure it's installed and "
            "available on your PYTHONPATH environment variable? Did you "
            "forget to activate a virtual environment?"
        ) from exc
    execute_from_command_line(sys.argv)


def django_main() -> None:
    from django.core.management import execute_from_command_line

    os.environ.setdefault("DJANGO_SETTINGS_MODULE", "cloud.mdb.ui.internal.mdbui.settings")
    os.environ.setdefault("PGSSLROOTCERT", "/opt/yandex/ui/allCAs.pem")
    execute_from_command_line(sys.argv)
