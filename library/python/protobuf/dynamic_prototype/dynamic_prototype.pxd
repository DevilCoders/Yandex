from libcpp cimport bool

from util.generic.string cimport TString
from util.generic.ptr cimport TIntrusivePtr


cdef extern from "google/protobuf/descriptor.h" namespace "NProtoBuf":
    cdef cppclass Descriptor:
        pass


cdef extern from "google/protobuf/message.h" namespace "NProtoBuf":
    cdef cppclass Message:
        TString SerializeAsString() const
        bool ParseFromString(const TString&)


cdef extern from "google/protobuf/descriptor.pb.h" namespace "NProtoBuf":
    cdef cppclass FileDescriptorSet:
        bool ParseFromString(const TString&)


cdef extern from "library/cpp/protobuf/dynamic_prototype/dynamic_prototype.h" nogil:
    cdef cppclass TDynamicMessage "TDynamicPrototype::TDynamicMessage":
        Message* Get()

    cdef cppclass TDynamicPrototype:
        @staticmethod
        TIntrusivePtr[TDynamicPrototype] StaticCreate "Create" (const FileDescriptorSet& fds, const TString& messageName)

        TDynamicMessage Create()
        const Descriptor* GetDescriptor() const;


cdef TIntrusivePtr[TDynamicPrototype] get_dynamic_prototype(proto_type)
cdef TDynamicMessage get_dynamic_message(proto_type)
