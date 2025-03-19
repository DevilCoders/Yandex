from kernel.blender.factor_storage.protos.storage_pb2 import TFactorStorages as ProtoStorages
from kernel.blender.factor_storage.pylib import compression

from six import iteritems


def compress_proto(proto_storages, base64_encode=True):
    return compression.compress_string(proto_storages.SerializeToString(), base64_encode)


def compress(static_factors, dynamic_factors, base64_encode=True):
    proto_storages = ProtoStorages()
    proto_storages.StaticStorage.Value.extend(static_factors)
    for key, value in iteritems(dynamic_factors):
        dynamic_factor_group = proto_storages.DynamicStorage.DynamicFactorGroup.add()
        dynamic_factor_group.Key = key
        dynamic_factor_group.Value.extend(value)
    return compress_proto(proto_storages, base64_encode)


def decompress_proto(compressed_str, base64_decode=True):
    proto_storages = ProtoStorages()
    try:
        decompressed = compression.decompress_string(compressed_str, base64_decode)
    except Exception as e:
        return str(e), proto_storages
    if not proto_storages.ParseFromString(decompressed):
        return "Cant parse protobuf message", proto_storages
    return None, proto_storages


def decompress(compressed_str, base64_decode=True):
    error, proto_storages = decompress_proto(compressed_str, base64_decode)
    if error is not None:
        return error, [], {}
    static_factors = list(proto_storages.StaticStorage.Value)
    dynamic_factors = {}
    for group in proto_storages.DynamicStorage.DynamicFactorGroup:
        dynamic_factors[group.Key] = list(group.Value)
    return None, static_factors, dynamic_factors
