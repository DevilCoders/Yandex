from mstand_structs import SqueezeVersions
from mstand_structs import LampKey
from yaqutils import DateRange


class TestLampKey(object):
    def test_lamp_key_serialize(self):
        dates = DateRange(start="20180101", end="20180102")
        lamp_key = LampKey(testid="100500", control="9001", observation="111", dates=dates,
                           version=SqueezeVersions(service_versions={}, common=None))
        lamp_key_ser = lamp_key.serialize()
        lamp_key_deser = LampKey.deserialize(lamp_key_ser)
        assert lamp_key_deser.testid == "100500"
