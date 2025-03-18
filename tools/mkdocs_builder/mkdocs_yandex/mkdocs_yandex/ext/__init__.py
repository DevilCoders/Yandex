import sys

try:
    # python 2
    reload(sys)  # noqa
    sys.setdefaultencoding('utf8')
except:
    # python 3
    pass
