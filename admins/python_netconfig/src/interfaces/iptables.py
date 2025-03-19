from interfaces.interface_action import InterfaceAction


class Iptables(InterfaceAction):
    def __init__(self):
        super(Iptables, self).__init__()
        self._table = None
        self._action = None
        self._chain = None
        self._filter = None
        self._rule_action = None
        self._priority = None
        self._raw = None

    def insert(self):
        self._action = "-I"
        return self

    def append(self):
        self._action = "-A"
        return self

    def delete(self):
        self._action = "-D"
        return self

    def filter(self, _filter):
        self._filter = _filter
        return self

    def rule_action(self, _act):
        self._rule_action = "-j " + _act
        return self

    def raw(self, _raw):
        self._raw = _raw
        return self

    def priority(self, priority):
        self._priority = priority
        return self

    def table(self, table):
        self._table = table
        return self

    def chain(self, chain):
        self._chain = chain
        return self

    def __str__(self):
        result = self.get_sbin_iptables()
        if self._table is not None:
            result += " -t " + str(self._table)
        if self._action is not None:
            result += " " + self._action
        else:
            result += " -A"
        if self._chain is not None:
            result += " " + self._chain
        else:
            result += " INPUT"
        if self._priority is not None:
            result += " " + str(self._priority)
        if self._filter is not None:
            result += " " + str(self._filter)
        if self._rule_action is not None:
            result += " " + str(self._rule_action)
        if self._raw is not None:
            result = self.get_sbin_iptables() + " " + str(self._raw)
        return result
