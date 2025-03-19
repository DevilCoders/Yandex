import collections

from schematics.types import StringType, ListType, ModelType
from yc_common.clients.models.grpc.common import GrpcModel
from yc_common.validation import LBPortType


class GrpcModelTest(collections.UserDict):
    def __init__(self, **kwargs):
        super(GrpcModelTest, self).__init__(**kwargs)
        for key, value in kwargs.items():
            setattr(self, key, value)

    def ByteSize(self):
        return True

    def __eq__(self, other):
        if len(self.keys()) != len(other.fields.keys()):
            for none_key in other.fields.keys() - self.keys():
                assert getattr(other, none_key, None) is None
        for key, value in self.items():
            assert value == getattr(other, key, None)
        return True


class GrpcHealthCheckTest(GrpcModel):
    @classmethod
    def get_grpc_class(cls):
        return GrpcModelTest

    name = StringType()


class GrpcTargetTest(GrpcModel):
    @classmethod
    def get_grpc_class(cls):
        return GrpcModelTest

    address = StringType()
    ports = ListType(LBPortType())
    health_checks = ListType(ModelType(GrpcHealthCheckTest))


class GrpcAddressesTest(GrpcModel):
    @classmethod
    def get_grpc_class(cls):
        return GrpcModelTest

    name = StringType()
    main_address = ModelType(GrpcTargetTest)
    addresses = ListType(ModelType(GrpcTargetTest))


def test_grpc_base_model_to_grpc():
    m = GrpcAddressesTest.new(
        name="name",
        main_address=GrpcTargetTest.new(
            address="127.0.0.1",
            ports=[22, 80],
            health_checks=[GrpcHealthCheckTest.new(name="qwe"), GrpcHealthCheckTest.new(name="asd")],
        ),
        addresses=[
            GrpcTargetTest.new(address="127.0.0.2", ports=[80, 443]),
            GrpcTargetTest.new(address="127.0.0.3", ports=[443, 8080]),
        ],
    )

    g = m.to_grpc()
    assert g == m
    m1 = GrpcAddressesTest.from_grpc(g)
    assert m == m1
