import re
import sys

from behave.__main__ import main as behave_main

from cloud.mdb.infratests.provision.provision import provision


def main():
    if sys.argv[1] == 'provision':
        provision()
    else:
        sys.argv[0] = re.sub(r'(-script\.pyw|\.exe)?$', '', sys.argv[0])
        sys.exit(behave_main())
