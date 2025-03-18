
from interface_action import InterfaceAction


class Route(InterfaceAction):
    def __init__(self):
        super(Route, self).__init__()
        self._action = None
        self._route = None
        self._mtu = None
        self._via = None
        self._table = None
        self._src = None
        self._dev = None
        self._advmss = None
        self._metric = None

    def add(self):
        self._action = "add"
        return self

    def replace(self):
        self._action = "replace"
        return self

    def delete(self):
        self._action = "del"
        return self

    def route(self, route):
        self._route = route
        return self

    def default(self):
        self._route = "default"
        return self

    def via(self, via):
        self._via = via
        return self

    def dev(self, dev):
        self._dev = dev
        return self

    def src(self, src):
        self._src = src
        return self

    def mtu(self, mtu):
        self._mtu = mtu
        return self

    def advmss(self, advmss):
        self._advmss = advmss
        return self

    def table(self, table):
        self._table = table
        return self

    def metric(self, metric):
        self._metric = metric
        return self

    def __str__(self):
        if self._ip_version == 6:
            head_length = 60
        else:
            head_length = 40
        result = self.get_sbin_ip() + " route "
        result += self._action + " " + self._route
        if self._via is not None:
            result += " via " + self._via
        if self._dev is not None:
            result += " dev " + self._dev
        if self._src is not None:
            result += " src " + self._src
        if self._mtu is not None:
            result += " mtu " + str(self._mtu)
        if self._advmss is not None:
            result += " advmss " + str(self._advmss)
        elif self._mtu is not None:
            result += " advmss " + str(self._mtu - head_length)
        if self._table is not None:
            result += " table " + str(self._table)
        if self._metric is not None:
            result += " metric " + str(self._metric)
        return result

    def __repr__(self):
        return str(self)
