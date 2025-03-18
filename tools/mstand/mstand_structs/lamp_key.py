from collections import OrderedDict
from mstand_structs import SqueezeVersions

import yaqutils.misc_helpers as umisc
from yaqutils import DateRange


@umisc.hash_and_ordering_from_key_method
class LampKey(object):
    def __init__(self, testid, control, observation, dates, version):
        """
        :type testid: str
        :type control: str
        :type observation: str
        :type dates: DateRange
        :type version: SqueezeVersions
        """

        self.testid = testid
        self.control = control
        self.observation = observation
        self.dates = dates
        self.version = version

    def pretty_name(self):
        return "LampKey(testid={}, control={}, obs={}, dates={}, version={})".format(self.testid,
                                                                                     self.control,
                                                                                     self.observation,
                                                                                     self.dates,
                                                                                     self.version)

    def serialize(self):
        result = OrderedDict()
        result["testid"] = self.testid
        result["control"] = self.control
        result["observation"] = self.observation
        result["dates"] = self.dates.serialize()
        result["version"] = self.version.serialize()

        return result

    @staticmethod
    def deserialize(lamp_key_data):
        testid = lamp_key_data["testid"]
        control = lamp_key_data["control"]
        observation = lamp_key_data["observation"]
        dates = DateRange.deserialize(lamp_key_data["dates"])
        version = SqueezeVersions.deserialize(lamp_key_data["version"])
        return LampKey(testid=testid, control=control, observation=observation, dates=dates, version=version)

    def __str__(self):
        return self.pretty_name()

    def __repr__(self):
        return str(self)

    def key(self):
        return self.testid, self.control, self.observation, self.dates
