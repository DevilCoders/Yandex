
from interfaces.interface_action import InterfaceAction


class Rule(InterfaceAction):
    def __init__(self):
        super(Rule, self).__init__()
        self._action = None
        self._iif = None
        self._from = None
        self._oif = None
        self._to = None
        self._fwmark = None
        self._lookup = None
        self._priority = None
        self._table = None

    def add(self):
        self._action = "add"
        return self

    def delete(self):
        self._action = "del"
        return self

    def iif(self, _iif):
        self._iif = _iif
        return self

    def from_addr(self, _from):
        self._from = _from
        return self

    def fwmark(self, fwmark):
        self._fwmark = fwmark
        return self

    def oif(self, _oif):
        self._oif = _oif
        return self

    def to_addr(self, _to):
        self._to = _to
        return self

    def priority(self, priority):
        self._priority = priority
        return self

    def table(self, table):
        self._table = table
        return self

    def lookup(self, lookup):
        self._lookup = lookup
        return self

    def __str__(self):
        result = self.get_sbin_ip() + " rule " + self._action
        if self._from is not None:
            result += " from " + str(self._from)
        if self._iif is not None:
            result += " iif " + str(self._iif)
        if self._to is not None:
            result += " to " + str(self._to)
        if self._oif is not None:
            result += " oif " + str(self._oif)
        if self._fwmark is not None:
            result += " fwmark " + str(self._fwmark)
        if self._lookup is not None:
            result += " lookup " + str(self._lookup)
        if self._priority is not None:
            result += " priority " + str(self._priority)
        if self._table is not None:
            result += " table " + str(self._table)
        return result
