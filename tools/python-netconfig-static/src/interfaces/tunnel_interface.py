
from interface import Interface


class TunnelInterface(Interface):
    def __init__(self, *args, **kwargs):
        super(TunnelInterface, self).__init__(*args)

        self._auto = True
        self._type = 'manual'
        self._do_default = False
        self._do_ethtool = False
        self.modprobe = kwargs['modprobe']

    def _get_up(self):
        up = super(TunnelInterface, self)._get_up()
        up.append('modprobe ' + self.modprobe + ' || true')
        up.append('ifconfig ' + self.name + ' up mtu ' + str(self._mtu))
        return up
    up = property(_get_up, Interface._set_up)

    def _get_down(self):
        down = super(TunnelInterface, self)._get_down()
        down.append('ifconfig ' + self.name + ' down')
        return down
    down = property(_get_down, Interface._set_down)
