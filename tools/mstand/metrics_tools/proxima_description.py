import copy
import logging
import re
from typing import Optional, List

DEFAULT_OWNER = "robot-proxima"
DEFAULT_URL = "svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia/quality/mstand_metrics/offline_production/metrics"
DEFAULT_MODULE = "albinkessel"
DEFAULT_CLASS = "AlbinKessel"

RE_CUSTOM_FORMULAS = re.compile(r"^D\.custom_formulas\['([0-9a-zA-Z_-]+)']$")


def confidence_key(confidence):
    return confidence["name"], confidence.get("yandex", False)


class ProximaPart(object):
    def __init__(
            self,
            confidences=None,
            requirements=None,
            label_script_parts=None,
            custom_formulas=None,
            custom_formulas_ap=None,
            signals=None,
            raw_signals=None,
            scales=None,
            multiplier=None,
            divisor=None,
            max_arg=None,
            aggregate_script=None,
    ):
        if confidences is None:
            confidences = list()
        if requirements is None:
            requirements = set()
        if label_script_parts is None:
            label_script_parts = list()
        if custom_formulas is None:
            custom_formulas = dict()
        if custom_formulas_ap is None:
            custom_formulas_ap = dict()
        if signals is None:
            signals = set()
        if raw_signals is None:
            raw_signals = set()
        if scales is None:
            scales = dict()

        self.scales = scales
        self.confidences = confidences
        self.custom_formulas = custom_formulas
        self.custom_formulas_ap = custom_formulas_ap
        self.signals = signals
        self.raw_signals = raw_signals
        self.requirements = requirements
        self.label_script_parts = label_script_parts
        self.multiplier = multiplier
        self.divisor = divisor
        self.max_arg = max_arg
        self.aggregate_script = aggregate_script

    def copy(self):
        return copy.deepcopy(self)

    @staticmethod
    def merge_list(parts):
        if not parts:
            return ProximaPart()
        result = ProximaPart()
        for part in parts:
            result.merge_with(part)
        result.validate()
        return result

    def merge_with(self, other: 'ProximaPart'):
        ProximaPart._strict_dict_update(self.scales, other.scales)
        ProximaPart._strict_dict_update(self.custom_formulas, other.custom_formulas)
        ProximaPart._strict_dict_update(self.custom_formulas_ap, other.custom_formulas_ap)

        self.signals.update(other.signals)
        self.raw_signals.update(other.raw_signals)
        self.requirements.update(other.requirements)
        self.label_script_parts.extend(other.label_script_parts)

        confidences = {confidence_key(c): c for c in self.confidences}
        ProximaPart._strict_dict_update(confidences, {confidence_key(c): c for c in other.confidences})
        self.confidences = sorted(confidences.values(), key=confidence_key)

        if other.multiplier:
            assert not self.multiplier
            self.multiplier = other.multiplier
        if other.divisor:
            assert not self.divisor
            self.divisor = other.divisor
        if other.max_arg:
            assert not self.max_arg
            self.max_arg = other.max_arg

        if other.aggregate_script:
            if self.aggregate_script is None:
                self.aggregate_script = other.aggregate_script
            else:
                assert self.aggregate_script == other.aggregate_script

    def to_mc_format(self, result):
        if self.scales:
            result["configuration"]["kwargs"]["scale_maps"] = self.scales

        if self.confidences:
            result["confidences"] = sorted(self.confidences, key=confidence_key)

        if self.custom_formulas:
            result["configuration"]["kwargs"]["custom_formulas"] = self.custom_formulas

        if self.custom_formulas_ap:
            result["configuration"]["kwargs"]["custom_formulas_after_precompute"] = self.custom_formulas_ap

        if self.signals:
            result["configuration"]["kwargs"]["signals"] = sorted(self.signals)

        if self.raw_signals:
            result["configuration"]["kwargs"]["raw_signals"] = sorted(self.raw_signals)

        result["configuration"]["requirements"] = sorted(self.requirements)

        label_script = self.create_label_script()
        if label_script:
            result["configuration"]["kwargs"]["label_script"] = label_script

        if self.aggregate_script:
            result["configuration"]["kwargs"]["aggregate_script"] = self.aggregate_script

        return result

    def create_label_script(self):
        label_script = None
        if self.label_script_parts:
            label_script = " + ".join(self.label_script_parts)
            if self.multiplier:
                label_script = "({}) * {}".format(label_script, self.multiplier)
            if self.max_arg:
                label_script = "max({}, {})".format(label_script, self.max_arg)
            if self.divisor:
                label_script = "({}) / {}".format(label_script, self.divisor)
        return label_script

    def formula_for_doc(self):
        if not self.label_script_parts:
            return ""
        result = " + ".join(ProximaPart._label_part_for_doc(p) for p in self.label_script_parts)
        if self.multiplier:
            result = "({}) * {}".format(result, self.multiplier)
        return result

    @staticmethod
    def _label_part_for_doc(part):
        if "**" in part:
            return part
        return " * ".join(ProximaPart._label_sub_part_for_doc(p) for p in part.split("*"))

    @staticmethod
    def _label_sub_part_for_doc(part):
        part = part.strip()
        match = RE_CUSTOM_FORMULAS.match(part)
        if match is None:
            return part
        else:
            return match.group(1)

    @staticmethod
    def _strict_dict_update(d1, d2):
        for key, value in d2.items():
            if key in d1:
                assert d1[key] == value, "{} mismatch:\n{} != {}".format(key, d1[key], value)
            else:
                d1[key] = copy.deepcopy(value)

    def validate(self):
        for formula in self.custom_formulas:
            try:
                eval(self.custom_formulas[formula])
            except NameError:
                pass
            except SyntaxError:
                logging.error("%s: syntax error", formula)
                raise

    def update_confidence(self, name, threshold):
        for confidence in self.confidences:
            if confidence["name"] == name:
                confidence["threshold"] = threshold


class ProximaDescription(object):
    # TODO: inherit or use omglib.MetricDescription
    def __init__(
            self,
            name,
            parts,
            revision,
            judged=False,
            greater_better=True,
            description=None,
            owner=None,
            depth=5,
            max_depth=10,
            aggregate_script=None,
            requirements=None,
            wiki=None,
            use_py3=True,
    ):
        self.name = name
        self.part = ProximaPart.merge_list(parts)
        self.greater_better = greater_better
        self.depth = depth
        self.max_depth = max_depth
        self.use_py3 = use_py3

        assert self.depth <= self.max_depth

        descr_parts = []
        if description:
            descr_parts.append(description)
        formula = self.part.formula_for_doc()
        if formula:
            descr_parts.append(formula)
        if wiki:
            descr_parts.append(wiki)
        self.description = u"\n\n".join(descr_parts)

        if owner is None:
            owner = DEFAULT_OWNER
        self.owner = owner

        self.judged = judged
        if self.judged:
            for scale_name, scale in self.part.scales.items():
                if "__NOT_JUDGED_DEFAULT__" in scale and scale.get("__SKIP_NOT_JUDGED__", True):
                    del scale["__NOT_JUDGED_DEFAULT__"]
                if not scale:
                    del self.part.scales[scale_name]
        else:
            for scale_name, scale in self.part.scales.items():
                if "__SKIP_NOT_JUDGED__" in scale:
                    del scale["__SKIP_NOT_JUDGED__"]
                if not scale:
                    del self.part.scales[scale_name]

        if aggregate_script:
            assert not self.part.aggregate_script
            self.part.aggregate_script = aggregate_script
        if requirements:
            self.part.requirements.update(requirements)

        self.revision = revision

    def to_mc_format(self):
        result = {
            "name": self.name,
            "type": "mstand",
            "greaterBetter": self.greater_better,
            "description": self.description,
            "deprecated": False,
            "configuration": {
                "revision": self.revision,
                "url": DEFAULT_URL,
                "module": DEFAULT_MODULE,
                "className": DEFAULT_CLASS,
                "kwargs": {
                    "judged": self.judged,
                    "depth": self.depth,
                    "max_depth": self.max_depth,
                    "signals": [],
                    "label_script": "0.0",
                    "scale_maps": {},
                },
                "requirements": [],
            },
            "owner": self.owner,
            "style": None,
            "responsibleUsers": [],
        }
        if self.use_py3 is not None:
            result["configuration"]["usePy3"] = self.use_py3
        self.part.to_mc_format(result)
        return result

    def copy(self):
        return copy.deepcopy(self)


class ProximaEngineDescription(object):
    def __init__(self, name, revision,
                 description=None,
                 owner=None,
                 depth=5,
                 max_depth=10,
                 target_wizard_types: Optional[List[int]] = None,
                 target_slices: Optional[List[str]] = None,
                 requirements=None,
                 wiki=None,
                 confidences: Optional[List[dict]] = None
                 ):
        self.name = name
        self.target_wizard_types = target_wizard_types
        self.target_slices = target_slices
        self.depth = depth
        self.max_depth = max_depth
        self.requirements = requirements or []
        self.confidences = confidences or []

        assert self.depth <= self.max_depth

        descr_lines = []
        if description:
            descr_lines.append(description)
        if wiki:
            descr_lines.append(wiki)
        self.description = "\n\n".join(descr_lines)

        if owner is None:
            owner = DEFAULT_OWNER
        self.owner = owner
        self.revision = revision

    def to_mc_format(self):

        kwargs = {
            "judged": False,
            "depth": self.depth,
            "max_depth": self.max_depth
        }
        if self.target_slices:
            kwargs["target_slices"] = self.target_slices
        if self.target_wizard_types:
            kwargs["target_wizard_types"] = self.target_wizard_types

        configuration = {
            "revision": self.revision,
            "url": DEFAULT_URL,
            "module": "proxima_metrics",
            "className": "Proxima2020Impact",
            "kwargs": kwargs
        }
        if self.requirements:
            configuration["requirements"] = self.requirements

        result = {
            "name": self.name,
            "type": "mstand",
            "greaterBetter": True,
            "description": self.description,
            "deprecated": False,
            "configuration": configuration,
            "owner": self.owner,
            "style": None,
            "responsibleUsers": [],
        }
        if self.confidences:
            result["confidences"] = self.confidences

        result["configuration"]["usePy3"] = True

        return result

    def copy(self):
        return copy.deepcopy(self)
