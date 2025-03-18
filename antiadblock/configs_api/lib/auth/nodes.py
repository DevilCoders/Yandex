class Node(object):
    """
    permission system node represent point which can be used to bind permission mask. E.g. to grant admin permissions on
    service 'some_service' you should bind 'admin' permission mask to node '*#services#some_service' aka ROOT.SERVICES['some_service']

    Doctests:
    TODO: TO BE DONE
    """

    def get_upper_nodes(self):
        nodes = []
        growing_node = ""
        for edge in self.base.split("#"):
            if growing_node:
                growing_node += "#"
            growing_node += edge
            nodes.append(Node(growing_node))
        return nodes

    def __init__(self, base):
        self.base = base

    def __str__(self):
        return self.base

    def __eq__(self, o):
        if isinstance(o, basestring):
            return self.base == o
        return self.base == o.base

    def __ne__(self, o):
        if isinstance(o, basestring):
            return self.base != o
        return self.base != o.base

    def __getattr__(self, attr):
        join_part = attr if attr[0] == '#' else '#' + attr
        return Node(self.base + join_part)

    def __getitem__(self, item):
        return self.__getattr__(str(item))

    def __repr__(self):
        return 'Node("{}")'.format(self.base)

    def __len__(self):
        return len(self.base)


class Root(Node):
    SERVICES = Node("*")["services"]


ROOT = Root("*")
