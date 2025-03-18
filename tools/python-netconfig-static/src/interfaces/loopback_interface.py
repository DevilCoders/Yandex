
from interface import Interface


class LoopbackInterface(Interface):
    def __init__(self, *args, **kwargs):
        super(LoopbackInterface, self).__init__(*args, **kwargs)
        self._name = 'lo'
        self._type = 'loopback'
        self.do_default = False
        self.do_ethtool = False
