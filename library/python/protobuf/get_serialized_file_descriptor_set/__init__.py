import google.protobuf.descriptor_pb2 as proto_descr_pb2


def get_file_descriptor_set(message_type):
    seen = set()
    fds = proto_descr_pb2.FileDescriptorSet()

    def collect_dependencies(file_desc):
        if file_desc.name in seen:
            return
        seen.add(file_desc.name)
        for dep in file_desc.dependencies:
            collect_dependencies(dep)

        file_desc_proto = fds.file.add()
        file_desc.CopyToProto(file_desc_proto)

    collect_dependencies(message_type.DESCRIPTOR.file)
    return fds


def get_serialized_file_descriptor_set(message_type):
    return get_file_descriptor_set(message_type).SerializeToString()
