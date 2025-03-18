
import os

from formats import Debian, Redhat


format = 'ubuntu'

__FORMATS = {
    'ubuntu': Debian,
    'redhat': Redhat,
}


def _get_formatter():
    try:
        return __FORMATS[format]
    except KeyError:
        raise ValueError(
            "No formatter defined for format `{0}'".format(format))


def write_all(filename, interfaces):
    interfaces = _get_formatter().as_string(interfaces)
    if filename is None:
        print interfaces
        return

    if os.path.isfile(filename):
        os.rename(filename, filename + ".backup")
    f = open(filename, "w")
    f.write(interfaces)
    f.close()
