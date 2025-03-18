from util.generic.string cimport TString

from util.system.types cimport ui64

cdef extern from "library/cpp/tf/graph_processor_base/graph_def_multiproto/graph_def_multiproto.h" namespace "NFTMoon":
    TString MultiProtoHeader(ui64 numNodes)
    TString MultiProtoPart(TString node)

def MultiProtoHeaderPy(ui64 numNodes):
    return MultiProtoHeader(numNodes)

def MultiProtoPartPy(TString node):
    return MultiProtoPart(node)

def ToMultiProto(graph_def):
    assert(len(graph_def.versions.SerializeToString()) == 0)
    assert(graph_def.version == 0)
    assert(len(graph_def.library.SerializeToString()) == 0)

    multiserialized = MultiProtoHeaderPy(len(graph_def.node))
    for node in graph_def.node:
        serialized = node.SerializeToString()
        multiserialized += MultiProtoPartPy(serialized)
    return multiserialized
