import six
from util.generic.string cimport *


cdef extern from "kernel/ugc/security/lib/record_identifier.h" namespace "NUgc::NSecurity":
    cdef cppclass TRecordIdentifierGenerator:
        TRecordIdentifierGenerator(const TString& ns, const TString& userId) except +
        TRecordIdentifierGenerator& ContextId(const TString& contextId) except +
        TRecordIdentifierGenerator& ParentId(const TString& parentId) except +
        TRecordIdentifierGenerator& ClientEntropy(const TString& clientEntropy) except +
        TString Build() except +;


cdef class RecordIdentifierGenerator:
    cdef TRecordIdentifierGenerator* generator

    def __cinit__(self, namespace_, user_id_):
        self.generator = new TRecordIdentifierGenerator(namespace_, user_id_)

    def add_context_id(self, context_id):
        self.generator.ContextId(context_id)

    def add_parent_id(self, parent_id):
        self.generator.ParentId(parent_id)

    def add_client_entropy(self, client_entropy):
        self.generator.ClientEntropy(client_entropy)

    def build(self):
        return self.generator.Build()

    def __dealloc__(self):
        del self.generator


cdef inline TString pystr_to_string(val) except +:
    if isinstance(val, six.binary_type):
        return val
    elif isinstance(val, six.string_types):
        return val.encode("utf8")
    raise TypeError("str or unicode expected")


def generate_identifier(ns, user_id, **kwargs):
    cdef TString namespace_
    cdef TString user_id_
    cdef TString context_id_
    cdef TString parent_id_
    cdef TString client_entropy_

    namespace_ = pystr_to_string(ns)
    user_id_ = pystr_to_string(user_id)

    generator = RecordIdentifierGenerator(namespace_, user_id_)

    if "context_id" in kwargs:
        context_id_ = pystr_to_string(kwargs["context_id"])
        generator.add_context_id(context_id_)

    if "parent_id" in kwargs:
        parent_id_ = pystr_to_string(kwargs["parent_id"])
        generator.add_parent_id(parent_id_)

    if "client_entropy" in kwargs:
        client_entropy_ = pystr_to_string(kwargs["client_entropy"])
        generator.add_client_entropy(client_entropy_)

    result = generator.build()
    if isinstance(ns, six.binary_type):
        return result
    return result.decode("utf-8")
