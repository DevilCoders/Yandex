from collections import OrderedDict

import yaqutils.misc_helpers as umisc
from user_plugins import PluginKey


class Deviations(object):
    def __init__(self, std_control=None, std_exp=None, std_diff=None):
        """
        :type std_control: float | None
        :type std_exp: float | None
        :type std_diff: float | None
        """
        self.std_control = std_control
        self.std_exp = std_exp
        self.std_diff = std_diff

    def serialize(self):
        result = OrderedDict()
        if self.std_control is not None:
            result["std_control"] = umisc.serialize_float(self.std_control)
        if self.std_exp is not None:
            result["std_exp"] = umisc.serialize_float(self.std_exp)
        if self.std_diff is not None:
            result["std_diff"] = umisc.serialize_float(self.std_diff)
        return result

    @staticmethod
    def deserialize(json_data):
        std_control = json_data.get("std_control")
        std_exp = json_data.get("std_exp")
        std_diff = json_data.get("std_diff")
        return Deviations(std_control=std_control,
                          std_exp=std_exp,
                          std_diff=std_diff)

    def __str__(self):
        return "StD(cont={}, exp={}, diff={})".format(self.std_control,
                                                      self.std_exp,
                                                      self.std_diff)

    def __repr__(self):
        return str(self)


class CriteriaResult(object):
    def __init__(self, criteria_key, pvalue, deviations=None, extra_data=None, synthetic=False):
        """
        :type criteria_key: PluginKey
        :type pvalue: float | None
        :type extra_data: object
        :type synthetic: bool
        """
        self.criteria_key = criteria_key
        self.pvalue = pvalue
        self.deviations = deviations
        self.extra_data = extra_data
        self.synthetic = synthetic
        self.color = None

    def serialize(self):
        """
        :rtype: dict
        """
        if self.synthetic:
            return self.pvalue
        else:
            result = OrderedDict()
            result["pvalue"] = umisc.serialize_float(self.pvalue)

            if self.deviations:
                result["deviations"] = self.deviations.serialize()

            criteria_key = self.criteria_key.serialize()
            result["criteria_key"] = criteria_key

            if self.extra_data is not None:
                result["extra_data"] = self.extra_data

            return result

    @staticmethod
    def deserialize(crit_res_data):
        """
        :type crit_res_data: dict
        :rtype: CriteriaResult
        """
        name = crit_res_data.get("name")
        if name:
            # logging.info("Old criteria key format")
            criteria_key = PluginKey(name=name)
        else:
            # logging.info("New criteria key format")
            criteria_key = PluginKey.deserialize(crit_res_data["criteria_key"])

        pvalue = crit_res_data["pvalue"]
        if pvalue is not None and not umisc.is_number(pvalue):
            raise Exception("Incorrect type of 'pvalue' field (expected number). Value: {}".format(pvalue))

        deviations_data = crit_res_data.get("deviations")
        if deviations_data is not None:
            deviations = Deviations.deserialize(deviations_data)
        else:
            deviations = None

        extra_data = crit_res_data.get("extra_data")
        return CriteriaResult(criteria_key=criteria_key,
                              pvalue=pvalue,
                              deviations=deviations,
                              extra_data=extra_data)

    def __str__(self):
        if self.extra_data:
            return "CritRes({}, pv={}, extra={})".format(self.criteria_key, self.pvalue, self.extra_data)
        else:
            return "CritRes({}, pv={})".format(self.criteria_key, self.pvalue)

    def __repr__(self):
        return str(self)

    def __eq__(self, other):
        raise Exception("CriteriaResult is not comparable")

    def __hash__(self):
        raise Exception("CriteriaResult is not hashable")

    def key(self):
        return self.criteria_key
