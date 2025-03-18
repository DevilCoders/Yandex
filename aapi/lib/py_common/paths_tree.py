class PathsTree(object):

    def __init__(self, paths=None):
        self._root = {'children': {}}
        if paths:
            for p in paths:
                self.add_path(p)

    def add_path(self, path, value=None):
        cur = self._root
        for p in path.split('/'):
            if p not in cur['children']:
                cur['children'][p] = {'children': {}}
            cur = cur['children'][p]
        cur['value'] = value

    def traverse(self, before=None, after=None, deps_filter=None):
        prefix = []

        def visit(node):
            if before:
                before(prefix, node)

            for c_name, c in node['children'].iteritems():
                if deps_filter and not deps_filter(prefix, c_name, c):
                    continue
                prefix.append(c_name)
                visit(c)
                prefix.pop()

            if after:
                after(prefix, node)

        visit(self._root)

    def show(self, value_printer=None):

        def before(prefix, node):
            p = '/'.join(prefix)
            if 'value' in node:
                v = value_printer(node['value']) if value_printer else str(node['value'])
                print prefix, p, v
            else:
                print prefix, p

        self.traverse(before=before)

    def root(self):
        return self._root
