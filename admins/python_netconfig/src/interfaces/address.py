from interface_action import InterfaceAction


class Address(InterfaceAction):
    def __init__(self):
        super(Address, self).__init__()
        self._action = None
        self._dev = None
        self._address = None
        self._label = None

    def add(self):
        self._action = "add"
        return self

    def delete(self):
        self._action = "del"
        return self

    def address(self, address):
        self._address = address
        return self

    def dev(self, dev):
        self._dev = dev
        return self

    def label(self, label):
        self._label = label
        return self

    def __str__(self):
        result = self.get_sbin_ip() + " address "
        result += self._action + " " + self._address
        result += " dev " + self._dev
        if self._label:
            result += ' label ' + self._label
        return result
