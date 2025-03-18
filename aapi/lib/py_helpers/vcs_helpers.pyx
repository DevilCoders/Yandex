from util.generic.string cimport TString
from util.generic.vector cimport TVector
from util.system.types cimport ui8, ui32, ui64
import hashlib


cdef extern from "aapi/lib/py_helpers/helpers.h" nogil:
    TString Compress(TString);
    TString Decompress(TString);
    void CompressFile(TString, TString);
    void DecompressFile(TString, TString);

    cdef cppclass TSvnCommit:
        TSvnCommit() except +
        ui32 revision
        TString author
        TString date
        TString msg
        TString tree
        TString parent

    cdef cppclass TTreeEntry:
        TTreeEntry() except +
        TString name
        ui8 mode
        ui64 size
        TString hash
        TVector[TString] blobs;

    cdef cppclass TTree:
        TTree() except +
        TVector[TTreeEntry] entries

    cdef cppclass THgHead:
        THgHead() except +
        TString branch
        TString hash

    cdef cppclass THgHeads:
        THgHeads() except +
        TVector[THgHead] heads;

    cdef cppclass THgChangeset:
        THgChangeset() except +
        TString hash
        TString author
        TString date
        TString msg
        TVector[TString] files
        TString branch
        ui8 close_branch
        TString extra
        TString tree
        TVector[TString] parents

    TString DumpSvnCommit(const TSvnCommit&);
    TSvnCommit LoadSvnCommit(const TString&);

    TString DumpTree(const TTree&);
    TTree LoadTree(const TString&);

    TString DumpHgHeads(const THgHeads&);
    THgHeads LoadHgHeads(const TString&);

    TString DumpHgChangeset(const THgChangeset&);
    THgChangeset LoadHgChangeset(const TString&);


cdef TString _to_TString(s):
    assert isinstance(s, basestring)
    if isinstance(s, unicode):
        s = s.encode('UTF-8')
    return TString(<const char*>s, len(s))


def compress(s):
    return Compress(_to_TString(s))


def decompress(s):
    return Decompress(_to_TString(s))


def compress_file(s, d):
    return CompressFile(_to_TString(s), _to_TString(d))


def decompress_file(s, d):
    return DecompressFile(_to_TString(s), _to_TString(d))


class SvnCommit(object):

    def __init__(self, revision, author, date, msg, tree, parent=None):
        self.revision = revision
        self.author = author
        self.date = date
        self.msg = msg
        self.tree = tree
        self.parent = parent or ''


class TreeEntry(object):

    def __init__(self, name, mode, size, hash, blobs):
        self.name = name
        self.mode = mode
        self.size = size
        self.hash = hash
        self.blobs = blobs


class Tree(object):

    def __init__(self, entries):
        self.entries = entries


class HgHead(object):

    def __init__(self, branch, hash):
        self.branch = branch
        self.hash = hash


class HgHeads(object):

    def __init__(self, heads):
        self.heads = heads


class HgChangeset(object):

    def __init__(self, hash, author, date, msg, files, branch, close_branch, extra, tree, parents):
        self.hash = hash
        self.author = author
        self.date = date
        self.msg = msg
        self.files = files
        self.branch = branch
        self.close_branch = close_branch
        self.extra = extra
        self.tree = tree
        self.parents = parents


cdef THgHead _pyobj_to_THgHead(obj):
    assert isinstance(obj, HgHead)

    cdef THgHead hg_head
    hg_head.branch = _to_TString(obj.branch)
    hg_head.hash = _to_TString(obj.hash)

    return hg_head


cdef _THgHead_to_pyobj(THgHead head):
    return HgHead(
        head.branch,
        head.hash
    )


cdef THgHeads _pyobj_to_THgHeads(obj):
    assert isinstance(obj, HgHeads)

    cdef THgHeads heads
    heads.heads = TVector[THgHead]()
    for h in obj.heads:
        heads.heads.push_back(_pyobj_to_THgHead(h))

    return heads


cdef _THgHeads_to_pyobj(THgHeads heads):
    hs = []
    for i in xrange(heads.heads.size()):
        hs.append(_THgHead_to_pyobj(heads.heads[i]))

    return HgHeads(hs)


cdef THgChangeset _pyobj_to_THgChangeset(obj):
    assert isinstance(obj, HgChangeset)

    cdef THgChangeset changeset;
    changeset.hash = _to_TString(obj.hash)
    changeset.author = _to_TString(obj.author)
    changeset.date = _to_TString(obj.date)
    changeset.msg = _to_TString(obj.msg)

    changeset.files = TVector[TString]()
    for f in obj.files:
        changeset.files.push_back(_to_TString(f))

    changeset.branch = _to_TString(obj.branch)
    changeset.close_branch = <ui8>obj.close_branch
    changeset.extra = _to_TString(obj.extra)
    changeset.tree = _to_TString(obj.tree)

    changeset.parents = TVector[TString]()
    for p in obj.parents:
        changeset.parents.push_back(_to_TString(p))

    return changeset


cdef _THgChangeset_to_pyobj(THgChangeset changeset):
    return HgChangeset(
        changeset.hash,
        changeset.author,
        changeset.date,
        changeset.msg,
        changeset.files,
        changeset.branch,
        changeset.close_branch,
        changeset.extra,
        changeset.tree,
        changeset.parents
    )


cdef TTree _pyobj_to_TTree(obj):
    assert isinstance(obj, Tree)

    cdef TTree tree
    tree.entries = TVector[TTreeEntry]()
    for e in obj.entries:
        tree.entries.push_back(_pyobj_to_TTreeEntry(e))

    return tree


cdef _TTree_to_pyobj(TTree tree):
    entries = []
    for i in xrange(tree.entries.size()):
        entries.append(_TTreeEntry_to_pyobj(tree.entries[i]))

    return Tree(
        entries
    )


cdef TTreeEntry _pyobj_to_TTreeEntry(obj):
    assert isinstance(obj, TreeEntry)

    cdef TTreeEntry entry;
    entry.name = _to_TString(obj.name)
    entry.mode = <ui8>obj.mode
    entry.size = <ui64>obj.size
    entry.hash = _to_TString(obj.hash)

    entry.blobs = TVector[TString]()
    for blob in obj.blobs:
        entry.blobs.push_back(_to_TString(blob))

    return entry


cdef _TTreeEntry_to_pyobj(TTreeEntry entry):
    return TreeEntry(
        entry.name,
        entry.mode,
        entry.size,
        entry.hash,
        entry.blobs
    )


cdef TSvnCommit _pyobj_to_TSvnCommit(obj):
    assert isinstance(obj, SvnCommit)

    cdef TSvnCommit commit;
    commit.revision = <ui32>obj.revision
    commit.author = _to_TString(obj.author)
    commit.date = _to_TString(obj.date)
    commit.msg = _to_TString(obj.msg)
    commit.tree = _to_TString(obj.tree)
    commit.parent = _to_TString(obj.parent)

    return commit


cdef _TSvnCommit_to_pyobj(TSvnCommit commit):
    return SvnCommit(
        commit.revision,
        commit.author,
        commit.date,
        commit.msg,
        commit.tree,
        commit.parent
    )


def dump_svn_commit_fbs(obj):
    return DumpSvnCommit(_pyobj_to_TSvnCommit(obj))


def load_svn_commit_fbs(s):
    return _TSvnCommit_to_pyobj(LoadSvnCommit(_to_TString(s)))


def dump_tree_fbs(obj):
    return DumpTree(_pyobj_to_TTree(obj))


def load_tree_fbs(s):
    return _TTree_to_pyobj(LoadTree(_to_TString(s)))


def dump_hg_heads_fbs(obj):
    return DumpHgHeads(_pyobj_to_THgHeads(obj))


def load_hg_heads_fbs(s):
    return _THgHeads_to_pyobj(LoadHgHeads(_to_TString(s)))


def dump_hg_changeset_fbs(obj):
    return DumpHgChangeset(_pyobj_to_THgChangeset(obj))


def load_hg_changeset_fbs(s):
    return _THgChangeset_to_pyobj(LoadHgChangeset(_to_TString(s)))


def git_like_hash(s):
    sha = hashlib.sha1()
    sha.update(s + '\0' + str(len(s)))
    return sha


def hexdump(s):

    def byte_string(c):
        bs = '{0:x}'.format(ord(c))
        if len(bs) == 1:
            bs = '0' + bs
        return bs

    return ''.join([byte_string(c) for c in s])


def is_hex(s):
    try:
        s.decode('hex')
    except Exception:
        return False
    return True
