from interface import Interface


class BridgeInterface(Interface):
    def __init__(self, *args, **kwargs):
        super(BridgeInterface, self).__init__(*args, **kwargs)

        self._auto = True
        self._type = 'manual'
        self._bridge_maxwait = None
        self._bridge_fd = None

    def _get_bridge_maxwait(self):
        return self._bridge_maxwait

    def _set_bridge_maxwait(self, value):
        self._bridge_maxwait = value

    bridge_maxwait = property(_get_bridge_maxwait, _set_bridge_maxwait)

    def _get_bridge_fd(self):
        return self._bridge_fd

    def _set_bridge_fd(self, value):
        self._bridge_fd = value

    bridge_fd = property(_get_bridge_fd, _set_bridge_fd)
