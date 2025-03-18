

class InterfacesGroup(object):
    def __init__(self):
        self._interfaces = []

    def __iter__(self):
        return self._interfaces.__iter__()

    def interfaces(self):
        return self._interfaces

    def add_interface(self, interface):
        self._interfaces.append(interface)

    def add_interfaces(self, interfaces):
        self._interfaces += interfaces

    def remove_interface(self, interface):
        self._interfaces.remove(interface)

    def __str__(self):
        result = ""
        bridges = []
        for i in self._interfaces:
            if i.is_bridge() and i.bridge_ports in bridges:
                i.render_bridge_params = False
            result += str(i) + "\n"
            if i.is_bridge():
                bridges.append(i.bridge_ports)
        return result

    def __repr__(self):
        return "<InterfacesGroup: %s>" % repr(self._interfaces)
