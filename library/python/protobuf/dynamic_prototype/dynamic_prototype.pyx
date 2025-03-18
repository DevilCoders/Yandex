from cython.operator cimport dereference

from library.python.protobuf.get_serialized_file_descriptor_set import get_serialized_file_descriptor_set
import six
from util.generic.string cimport TString
from util.generic.ptr cimport TIntrusivePtr


cdef TIntrusivePtr[TDynamicPrototype] get_dynamic_prototype(proto_type):
    serialized_file_descriptor_set = get_serialized_file_descriptor_set(proto_type)
    cdef TString full_name = six.ensure_binary(proto_type.DESCRIPTOR.full_name)

    cdef FileDescriptorSet file_descriptor_set
    if not file_descriptor_set.ParseFromString(serialized_file_descriptor_set):
        raise Exception("Failed to transfer file descriptor set from Python to C++")

    return TDynamicPrototype.StaticCreate(file_descriptor_set, full_name)


cdef TDynamicMessage get_dynamic_message(proto_type):
    cdef TIntrusivePtr[TDynamicPrototype] prototype = get_dynamic_prototype(proto_type)
    return dereference(prototype.Get()).Create()
