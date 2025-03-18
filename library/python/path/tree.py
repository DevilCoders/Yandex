import six

from . import split


class Node(dict):
    def __init__(self, data=None):
        super(Node, self).__init__()
        self.alive = False
        self.data = data


class Tree(object):
    """
    Naive path trie suitable for file/directory tree traversals and subtree maniputaions.

    >>> Tree()  # empty tree
    >>> Tree(some_node)  # construct tree with root preset from some other tree's node

    Path tree is composed of nodes, where each node can be enabled or disabled and can store arbitrary payload.

    Path tree does not store paths (but it stores path components as keys) and therefore is not tied to any particular
    path type. Paths used with path tree methods must conform only to the single requirement: they must be iterable and
    produce path components on iteration.

    Path tree can be used as a path set or dict:

    >>> tree = Tree()
    >>> tree.set((u'some', u'dir'))
    >>> tree.set((u'some', u'dir', u'subdir', u'file'), 'some data')
    >>> tree.has((u'some', u'dir'))  # True
    >>> tree.has((u'some', u'dir', u'subdir'))  # False
    >>> tree.get((u'some', u'dir', u'subdir'))  # None
    >>> tree.get((u'some', u'dir', u'subdir', u'file'))  # 'some data'

    Basic traversal yields stored data for enabled nodes:

    >>> for data in tree:
    >>>     ...

    Path traversal:

    >>> for path, data in tree.traverse():
    >>>     ...


    """

    def __init__(self, node=None):
        self._root = node if node is not None else Node()

    def __iter__(self):
        return (path for path, data in self.traverse())

    def get(self, path):
        """
        Get path payload.

        See `Tree.get_node` for lower-level node-based method.

        :param path: path (any path components iterable)
        :return: payload or None (if path is not enabled or its payload is not set)
        """

        node = self.get_node(path)
        return node.data if (node is not None and node.alive) else None

    def has(self, path):
        """
        Check if path is set.

        :param path: path (any path components iterable)
        :return: True if path is set, False otherwise
        """

        node = self.get_node(path)
        return node is not None and node.alive

    def set(self, path, data=None, replace=True):
        """
        Set path.

        See `Tree.enable_node` for lower-level node-based method.

        :param path: path (any path components sequence)
        :param data: arbitrary payload
        :param replace: replace existing payload
        """

        node = self.enable_node(path, data, replace=replace)
        return node.data

    def traverse(self, descend=lambda path, node: True, path_type=split.Split):
        """
        Higher-level tree traversal (DFS).

        Traversal yields path and its payload for each enabled path. Output is sorted.

        See `Tree.traverse_nodes` for more details and for lower-level node-based method.

        :param descend: function of path and `Node` which is called for each node, even for non-active ones
                        must return True for traversal to descend further to node's children
        :param path_type: path type used to generate paths, this type must:
                          * be default-constructable
                          * be constructable from tuple of path components
                          * support addition
        """

        return ((path, node.data) for path, node in self.traverse_nodes(descend, path_type) if node.alive)

    def reduce(self, calc=lambda path, node, children: None, descend=lambda path, node: True):
        """
        Tree reduction.
        """

        class Op(object):
            def __init__(s, parent, node, path):
                s.parent = parent
                s.node = node
                s.path = path
                s.traversed = False
                s.input = []

            def run(s):
                s.parent.input.append(calc(s.path, s.node, s.input))

        class Result(Op):
            def __init__(s, root):
                super(Result, s).__init__(None, root, split.Split())
                s.value = None

            def run(s):
                s.value = calc(s.path, s.node, s.input)

        result = Result(self._root)
        stack = [result]
        while stack:
            elem = stack[-1]
            if elem.traversed:
                elem.run()
                stack.pop()
                continue
            elem.traversed = True
            if not descend(elem.path, elem.node):
                continue
            if elem.node:
                stack.extend(Op(elem, elem.node[name], elem.path + split.Split((name,))) for name in sorted(six.iterkeys(elem.node), reverse=True))

        return result.value

    def get_prefix(self, path):
        """
        Get payload for the deepest enabled path prefix.

        Path itself is also treated as its prefix.

        :param path: path (slicable path components sequence)
        :return: payload or None (if enabled prefix is not found or its payload is not set)
        """

        prefix = None
        data = None
        for prefix, data in self.iter_prefixes(path):
            pass
        return prefix, data

    def iter_prefixes(self, path):
        """
        Walk path, traverse enabled paths from root.

        Yields path and its payload for each enabled path ancestor (dir), also yields the path itself in the end (if it is set).

        See `Tree.iter_node_prefixes` for lower-level node-based method.

        :param path: path (slicable path components sequence)
        """

        return ((prefix, node.data) for prefix, node in self.iter_node_prefixes(path) if node.alive)

    def get_node(self, path):
        """
        Get path node.

        See `Tree.get` and `Tree.has` for higher-level methods.

        :param path: path (any path components iterable)
        :return: `Node` or None (if path is not enabled)
        """

        node = self._root
        for name in path:
            node = node.get(name)
            if node is None:
                return None
        return node

    def enable_node(self, path, data=None, replace=True):
        """
        Enable node for path.

        See `Tree.set` for higher-level method.

        :param path: path (any path components sequence)
        :param data: arbitrary payload
        :param replace: replace existing payload
        """

        node = self._root
        for n in range(len(path)):
            if path[n] not in node:
                node[path[n]] = Node()
            node = node.get(path[n])
        if replace or not node.alive:
            node.alive = True
            node.data = data
        return node

    def disable_node(self, path):
        """
        Unset path. Disable node for path.

        `Tree.remove` is an alias.

        :param path: path (any path components iterable)
        """

        node = self.get_node(path)
        if node is None:
            return
        node.alive = False
        node.data = None

    remove = disable_node

    def traverse_nodes(self, descend=lambda path, node: True, path_type=split.Split):
        """
        Lower-level tree traversal (DFS).

        Traversal yields pairs (path, node) for every node (enabled or disabled) in a tree. Output is sorted.

        Traversal is customizable with `descend` function.

        Method generates paths on the fly. Tuple-like `split.Split` is used as path type by default.
        Client can specify desired path type.

        See `Tree.traverse` for higher-level method.

        :param descend: function of path and `Node` which is called for each node, even for non-active ones
                        must return True for traversal to descend further to node's children
        :param path_type: path type used to generate paths, this type must:
                          * be default-constructable
                          * be constructable from tuple of path components
                          * support addition
        """

        stack = [(self._root, path_type())]
        while stack:
            node, path = stack.pop()
            yield path, node
            if not descend(path, node):
                continue
            if node:
                stack.extend((node[name], path + path_type((name,))) for name in sorted(six.iterkeys(node), reverse=True))

    def iter_node_prefixes(self, path):
        """
        Walk path, traverse nodes from root.

        Yields pairs (path, node) for path ancestors (dirs, enabled or disabled), also yields the path itself in the end.

        See `Tree.iter_prefixes` for higher-level method.

        :param path: path (slicable path components sequence)
        """

        node = self._root
        yield path[:0], node
        for n in range(len(path)):
            node = node.get(path[n])
            if node is None:
                break
            yield path[:n + 1], node

    def get_subtree(self, path):
        node = self.get_node(path)
        if node is None:
            return None
        return Tree(node)

    def remove_subtree(self, path):
        if not path:
            self._root = Node()
            return self._root
        node = self._root
        target_parent = self._root
        target_child = path[0]
        for n in range(len(path) - 1):
            node = node.get(path[n])
            if node is None:
                return self._root
            if node.alive or len(node) > 1:
                target_parent = node
                target_child = path[n + 1]
        if path[-1] in node:
            target_parent.pop(target_child)
        return self._root


def dump(tree, output, repr_data=lambda x: x, indent=' '):
    """
    Dump tree (debug purposes mostly).

    :param tree: path tree
    :param output: output stream
    :param repr_data: function that implements repr for tree payload
    :param indent: indent used to draw tree
    """

    for path, node in tree.traverse_nodes():
        suffix = u': {}'.format(repr_data(node.data)) if node.alive else ''
        six.print_(u'{indent}{name}{suffix}'.format(
            indent=' ' * len(path),
            name=path[-1] if path else '/',
            suffix=suffix,
        ), file=output)
