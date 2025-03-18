cdef extern from "library/cpp/framing/format.h":
    cdef enum _EFormat 'NFraming::EFormat':
        _Protoseq 'NFraming::EFormat::Protoseq'
        _Lenval  'NFraming::EFormat::Lenval'


ProtoSeq = <int> _Protoseq
Lenval = <int> _Lenval
