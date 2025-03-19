import re
import sys

from behave.__main__ import main as behave_main


def main():
    sys.argv[0] = re.sub(r'(-script\.pyw|\.exe)?$', '', sys.argv[0])
    sys.exit(behave_main())
