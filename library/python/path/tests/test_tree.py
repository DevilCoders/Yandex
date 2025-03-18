import pytest

import library.python.path.tree as path_tree


class CustomPath(object):
    def __init__(self, seq=()):
        self.seq = seq

    def __eq__(self, other):
        if not isinstance(other, CustomPath):
            return False
        return self.seq == other.seq

    def __iter__(self):
        return iter(self.seq)

    def __len__(self):
        return len(self.seq)

    def __add__(self, other):
        if not isinstance(other, CustomPath):
            raise ValueError()
        return CustomPath(self.seq.__add__(other.seq))

    def __getitem__(self, v):
        res = self.seq.__getitem__(v)
        return CustomPath(res) if isinstance(v, slice) else res

    # CPython compat
    def __getslice__(self, i, j):
        return self.__getitem__(slice(i, j))


@pytest.fixture()
def sample_tree():
    tree = path_tree.Tree()
    return tree


def test_node():
    class SomeNode(path_tree.Node):
        def __init__(self, data=None):
            super(SomeNode, self).__init__(data)

    node = path_tree.Node()
    node = path_tree.Node(42)
    node = SomeNode()
    node = SomeNode(42)
    del node


def test_empty():
    tree = path_tree.Tree()

    assert list(iter(tree)) == []
    assert list(tree.traverse()) == []
    assert tree.get(()) is None
    assert tree.get(('abc',)) is None
    assert tree.get(('abc', 'def', 'ghi')) is None
    assert not tree.has(())
    assert not tree.has(('abc',))
    assert not tree.has(('abc', 'def', 'ghi'))
    assert tree.get_prefix(()) == (None, None)
    assert tree.get_prefix(('abc',)) == (None, None)
    assert tree.get_prefix(('abc', 'def', 'ghi')) == (None, None)
    assert list(tree.iter_prefixes(())) == []
    assert list(tree.iter_prefixes(('abc',))) == []
    assert list(tree.iter_prefixes(('abc', 'def', 'ghi'))) == []


def test_root():
    tree = path_tree.Tree()
    tree.set((), 42)

    assert list(iter(tree)) == [()]
    assert list(tree.traverse()) == [((), 42)]
    assert tree.get(()) == 42
    assert tree.get(('abc',)) is None
    assert tree.get(('abc', 'def', 'ghi')) is None
    assert tree.has(())
    assert not tree.has(('abc',))
    assert not tree.has(('abc', 'def', 'ghi'))
    assert tree.get_prefix(()) == ((), 42)
    assert tree.get_prefix(('abc',)) == ((), 42)
    assert tree.get_prefix(('abc', 'def', 'ghi')) == ((), 42)
    assert list(tree.iter_prefixes(())) == [((), 42)]
    assert list(tree.iter_prefixes(('abc',))) == [((), 42)]
    assert list(tree.iter_prefixes(('abc', 'def', 'ghi'))) == [((), 42)]


def test_name():
    tree = path_tree.Tree()
    tree.set(('abc',), 42)

    assert list(iter(tree)) == [('abc',)]
    assert list(tree.traverse()) == [(('abc',), 42)]
    assert tree.get(()) is None
    assert tree.get(('abc',)) == 42
    assert tree.get(('def',)) is None
    assert tree.get(('abc', 'def', 'ghi')) is None
    assert not tree.has(())
    assert tree.has(('abc',))
    assert not tree.has(('def',))
    assert not tree.has(('abc', 'def', 'ghi'))
    assert tree.get_prefix(()) == (None, None)
    assert tree.get_prefix(('abc',)) == (('abc',), 42)
    assert tree.get_prefix(('def',)) == (None, None)
    assert tree.get_prefix(('abc', 'def', 'ghi')) == (('abc',), 42)
    assert list(tree.iter_prefixes(())) == []
    assert list(tree.iter_prefixes(('abc',))) == [(('abc',), 42)]
    assert list(tree.iter_prefixes(('def',))) == []
    assert list(tree.iter_prefixes(('abc', 'def', 'ghi'))) == [(('abc',), 42)]


def test_path():
    tree = path_tree.Tree()
    tree.set(('abc', 'def'), 42)

    assert list(iter(tree)) == [('abc', 'def')]
    assert list(tree.traverse()) == [(('abc', 'def'), 42)]
    assert tree.get(()) is None
    assert tree.get(('abc',)) is None
    assert tree.get(('abc', 'def')) == 42
    assert tree.get(('abc', 'def', 'ghi')) is None
    assert not tree.has(())
    assert not tree.has(('abc',))
    assert tree.has(('abc', 'def'))
    assert not tree.has(('abc', 'def', 'ghi'))
    assert tree.get_prefix(()) == (None, None)
    assert tree.get_prefix(('abc',)) == (None, None)
    assert tree.get_prefix(('abc', 'def')) == (('abc', 'def'), 42)
    assert tree.get_prefix(('abc', 'def', 'ghi')) == (('abc', 'def'), 42)
    assert list(tree.iter_prefixes(())) == []
    assert list(tree.iter_prefixes(('abc',))) == []
    assert list(tree.iter_prefixes(('abc', 'def'))) == [(('abc', 'def'), 42)]
    assert list(tree.iter_prefixes(('abc', 'def', 'ghi'))) == [(('abc', 'def'), 42)]


def test_path_custom():
    tree = path_tree.Tree()
    tree.set(CustomPath(('abc', 'def')), 42)

    assert list(iter(tree)) == [('abc', 'def')]
    assert list(tree.traverse()) == [(('abc', 'def'), 42)]
    assert tree.get(()) is None
    assert tree.get(('abc',)) is None
    assert tree.get(('abc', 'def')) == 42
    assert tree.get(('abc', 'def', 'ghi')) is None
    assert not tree.has(())
    assert not tree.has(('abc',))
    assert tree.has(('abc', 'def'))
    assert not tree.has(('abc', 'def', 'ghi'))
    assert not tree.has(())
    assert not tree.has(('abc',))
    assert tree.has(('abc', 'def'))
    assert not tree.has(('abc', 'def', 'ghi'))
    assert tree.get_prefix(('abc',)) == (None, None)
    assert tree.get_prefix(('abc', 'def')) == (('abc', 'def'), 42)
    assert tree.get_prefix(('abc', 'def', 'ghi')) == (('abc', 'def'), 42)
    assert list(tree.iter_prefixes(())) == []
    assert list(tree.iter_prefixes(('abc',))) == []
    assert list(tree.iter_prefixes(('abc', 'def'))) == [(('abc', 'def'), 42)]
    assert list(tree.iter_prefixes(('abc', 'def', 'ghi'))) == [(('abc', 'def'), 42)]

    assert list(tree.traverse(path_type=CustomPath)) == [(CustomPath(('abc', 'def')), 42)]
    assert tree.get(CustomPath()) is None
    assert tree.get(CustomPath(('abc',))) is None
    assert tree.get(CustomPath(('abc', 'def'))) == 42
    assert tree.get(CustomPath(('abc', 'def', 'ghi'))) is None
    assert not tree.has(CustomPath())
    assert not tree.has(CustomPath(('abc',)))
    assert tree.has(CustomPath(('abc', 'def')))
    assert not tree.has(CustomPath(('abc', 'def', 'ghi')))
    assert not tree.has(CustomPath())
    assert not tree.has(CustomPath(('abc',)))
    assert tree.has(CustomPath(('abc', 'def')))
    assert not tree.has(CustomPath(('abc', 'def', 'ghi')))
    assert tree.get_prefix(CustomPath(('abc',))) == (None, None)
    assert tree.get_prefix(CustomPath(('abc', 'def'))) == (CustomPath(('abc', 'def')), 42)
    assert tree.get_prefix(CustomPath(('abc', 'def', 'ghi'))) == (CustomPath(('abc', 'def')), 42)
    assert list(tree.iter_prefixes(CustomPath())) == []
    assert list(tree.iter_prefixes(CustomPath(('abc',)))) == []
    assert list(tree.iter_prefixes(CustomPath(('abc', 'def')))) == [(CustomPath(('abc', 'def')), 42)]
    assert list(tree.iter_prefixes(CustomPath(('abc', 'def', 'ghi')))) == [(CustomPath(('abc', 'def')), 42)]


def test_multi():
    tree = path_tree.Tree()
    tree.set(('abc', '123'), 42)
    tree.set((), 43)
    tree.set(('abc', '012', 'def'), 44)

    assert list(iter(tree)) == [(), ('abc', '012', 'def'), ('abc', '123')]
    assert list(tree.traverse()) == [((), 43), (('abc', '012', 'def'), 44), (('abc', '123'), 42)]
    assert tree.get(()) == 43
    assert tree.get(('abc',)) is None
    assert tree.get(('abc', '012')) is None
    assert tree.get(('abc', '012', 'def')) == 44
    assert tree.get(('abc', '123')) == 42
    assert tree.get(('abc', '123', 'def')) is None
    assert tree.has(())
    assert not tree.has(('abc',))
    assert not tree.has(('abc', '012'))
    assert tree.has(('abc', '012', 'def'))
    assert tree.has(('abc', '123'))
    assert not tree.has(('abc', '123', 'def'))
    assert tree.get_prefix(()) == ((), 43)
    assert tree.get_prefix(('abc',)) == ((), 43)
    assert tree.get_prefix(('abc', '012')) == ((), 43)
    assert tree.get_prefix(('abc', '012', 'def')) == (('abc', '012', 'def'), 44)
    assert tree.get_prefix(('abc', '123')) == (('abc', '123'), 42)
    assert tree.get_prefix(('abc', '123', 'def')) == (('abc', '123'), 42)
    assert list(tree.iter_prefixes(())) == [((), 43)]
    assert list(tree.iter_prefixes(('abc',))) == [((), 43)]
    assert list(tree.iter_prefixes(('abc', '012'))) == [((), 43)]
    assert list(tree.iter_prefixes(('abc', '012', 'def'))) == [((), 43), (('abc', '012', 'def'), 44)]
    assert list(tree.iter_prefixes(('abc', '123'))) == [((), 43), (('abc', '123'), 42)]
    assert list(tree.iter_prefixes(('abc', '123', 'def'))) == [((), 43), (('abc', '123'), 42)]


def test_override():
    tree = path_tree.Tree()
    tree.set(('abc', 'def'), 41)
    tree.set(('abc', 'def'), 42)

    assert list(iter(tree)) == [('abc', 'def')]
    assert list(tree.traverse()) == [(('abc', 'def'), 42)]
    assert tree.get(()) is None
    assert tree.get(('abc',)) is None
    assert tree.get(('abc', 'def')) == 42
    assert tree.get(('abc', 'def', 'ghi')) is None
    assert not tree.has(())
    assert not tree.has(('abc',))
    assert tree.has(('abc', 'def'))
    assert not tree.has(('abc', 'def', 'ghi'))
    assert tree.get_prefix(()) == (None, None)
    assert tree.get_prefix(('abc',)) == (None, None)
    assert tree.get_prefix(('abc', 'def')) == (('abc', 'def'), 42)
    assert tree.get_prefix(('abc', 'def', 'ghi')) == (('abc', 'def'), 42)
    assert list(tree.iter_prefixes(())) == []
    assert list(tree.iter_prefixes(('abc',))) == []
    assert list(tree.iter_prefixes(('abc', 'def'))) == [(('abc', 'def'), 42)]
    assert list(tree.iter_prefixes(('abc', 'def', 'ghi'))) == [(('abc', 'def'), 42)]


def test_remove():
    tree = path_tree.Tree()
    tree.set(('abc', 'ghi'), 41)
    tree.set(('abc', 'def'), 42)
    tree.remove(('abc', 'ghi'))

    assert list(iter(tree)) == [('abc', 'def')]
    assert list(tree.traverse()) == [(('abc', 'def'), 42)]
    assert tree.get(()) is None
    assert tree.get(('abc',)) is None
    assert tree.get(('abc', 'def')) == 42
    assert tree.get(('abc', 'def', 'ghi')) is None
    assert not tree.has(())
    assert not tree.has(('abc',))
    assert tree.has(('abc', 'def'))
    assert not tree.has(('abc', 'def', 'ghi'))
    assert tree.get_prefix(()) == (None, None)
    assert tree.get_prefix(('abc',)) == (None, None)
    assert tree.get_prefix(('abc', 'def')) == (('abc', 'def'), 42)
    assert tree.get_prefix(('abc', 'def', 'ghi')) == (('abc', 'def'), 42)
    assert list(tree.iter_prefixes(())) == []
    assert list(tree.iter_prefixes(('abc',))) == []
    assert list(tree.iter_prefixes(('abc', 'def'))) == [(('abc', 'def'), 42)]
    assert list(tree.iter_prefixes(('abc', 'def', 'ghi'))) == [(('abc', 'def'), 42)]

    tree.set(('abc'), 43)
    tree.set((), 44)

    tree.remove(('abc'))

    assert list(tree.traverse()) == [((), 44), (('abc', 'def'), 42)]

    tree.remove(())
    tree.remove(('abc', 'def'))

    assert list(tree.traverse()) == []


def test_remove_subtree():
    tree = path_tree.Tree()
    tree.set(('abc',), 41)
    tree.set(('abc', 'def'), 42)
    tree.set(('abc', 'ghi', 'ghi1'), 43)
    tree.set(('abc', 'ghi', 'ghi2', 'ghi3'), 44)

    assert list(tree.traverse()) == [(('abc',), 41), (('abc', 'def'), 42), (('abc', 'ghi', 'ghi1'), 43), (('abc', 'ghi', 'ghi2', 'ghi3'), 44)]

    tree.remove_subtree(('abc', 'ghi'))

    assert list(iter(tree)) == [('abc',), ('abc', 'def')]
    assert list(tree.traverse()) == [(('abc',), 41), (('abc', 'def'), 42)]
    assert tree.get(('abc', 'ghi')) is None
    assert tree.get(('abc', 'ghi', 'ghi1')) is None
    assert tree.get(('abc', 'ghi', 'ghi2')) is None
    assert tree.get(('abc', 'ghi', 'ghi2', 'ghi3')) is None
    assert not tree.has(('abc', 'ghi'))
    assert not tree.has(('abc', 'ghi', 'ghi1'))
    assert not tree.has(('abc', 'ghi', 'ghi2'))
    assert not tree.has(('abc', 'ghi', 'ghi2', 'ghi3'))
    assert tree.get_prefix(('abc', 'ghi')) == (('abc',), 41)
    assert tree.get_prefix(('abc', 'ghi', 'ghi1')) == (('abc',), 41)
    assert tree.get_prefix(('abc', 'ghi', 'ghi2')) == (('abc',), 41)
    assert tree.get_prefix(('abc', 'ghi', 'ghi2', 'ghi3')) == (('abc',), 41)
    assert list(tree.iter_prefixes(('abc', 'ghi'))) == [(('abc',), 41)]
    assert list(tree.iter_prefixes(('abc', 'ghi', 'ghi1'))) == [(('abc',), 41)]
    assert list(tree.iter_prefixes(('abc', 'ghi', 'ghi2'))) == [(('abc',), 41)]
    assert list(tree.iter_prefixes(('abc', 'ghi', 'ghi2', 'ghi3'))) == [(('abc',), 41)]

    tree.remove_subtree(())

    assert list(tree.traverse()) == []


def test_traverse():
    class LoggingDescendHandler(object):
        def __init__(self):
            self.log = []

        def handle(self, path, node):
            self.log.append((path, node.alive, node.data))
            return True

    def no_descend(path, node):
        return False

    def forbid_descend_for(p):
        return lambda path, node: path != p

    tree = path_tree.Tree()
    tree.set(('abc', '123'), 42)
    tree.set((), 43)
    tree.set(('abc', '012', 'def'), 44)

    logging_descend_handler = LoggingDescendHandler()
    assert list(tree.traverse()) == [((), 43), (('abc', '012', 'def'), 44), (('abc', '123'), 42)]
    assert list(tree.traverse(descend=logging_descend_handler.handle)) == [((), 43), (('abc', '012', 'def'), 44), (('abc', '123'), 42)]
    assert logging_descend_handler.log == [((), True, 43), (('abc',), False, None), (('abc', '012'), False, None), (('abc', '012', 'def'), True, 44), (('abc', '123'), True, 42)]
    assert list(tree.traverse(descend=no_descend)) == [((), 43)]
    assert list(tree.traverse(descend=forbid_descend_for(('abc', '012')))) == [((), 43), (('abc', '123'), 42)]
    assert list(tree.traverse(descend=forbid_descend_for(('abc',)))) == [((), 43)]
    assert list(tree.traverse(descend=forbid_descend_for(()))) == [((), 43)]


def test_reduce():
    class LoggingDescendHandler(object):
        def __init__(self):
            self.log = []

        def handle(self, path, node):
            self.log.append((path, node.alive, node.data))
            return True

    class LoggingCalcHandler(object):
        def __init__(self):
            self.log = []

        def handle(self, path, node, children):
            self.log.append((path, children))
            return path.basename

    def count_elems(path, node, children):
        return sum(children) + (1 if node.alive else 0)

    def no_descend(path, node):
        return False

    def forbid_descend_for(p):
        return lambda path, node: path != p

    tree = path_tree.Tree()
    tree.set(('abc', '123'), 42)
    tree.set(('abc', '124'), 45)
    tree.set((), 43)
    tree.set(('abc', '012', 'def'), 44)

    logging_descend_handler = LoggingDescendHandler()
    logging_calc_handler = LoggingCalcHandler()
    assert tree.reduce() is None
    assert tree.reduce(calc=logging_calc_handler.handle, descend=logging_descend_handler.handle) is None
    assert logging_calc_handler.log == [
        (('abc', '012', 'def'), []),
        (('abc', '012'), ['def']),
        (('abc', '123'), []),
        (('abc', '124'), []),
        (('abc',), ['012', '123', '124']),
        ((), ['abc']),
    ]
    assert logging_descend_handler.log == [
        ((), True, 43),
        (('abc',), False, None),
        (('abc', '012'), False, None),
        (('abc', '012', 'def'), True, 44),
        (('abc', '123'), True, 42),
        (('abc', '124'), True, 45),
    ]
    assert tree.reduce(calc=count_elems) == 4
    assert tree.reduce(calc=count_elems, descend=no_descend) == 1
    assert tree.reduce(calc=count_elems, descend=forbid_descend_for(('abc',))) == 1
