import copy
from typing import Dict
from typing import Optional


# this is a generic, universal Metric description for Metrics service
class MetricDescription(object):
    def __init__(
            self,
            name,
            revision,
            owner,
            url,
            metric_module,
            metric_class,
            greater_better=True,
            description=None,
            wiki=None,
            requirements=None,
            kwargs=None,
            confidences=None,
            use_py3=None,
            responsible_users=None,
            style: Optional[Dict] = None,
    ):
        self.name = name
        self.greater_better = greater_better
        self.description = description
        self.wiki = wiki
        self.owner = owner
        self.url = url
        self.metric_module = metric_module
        self.metric_class = metric_class
        self.revision = revision
        self.requirements = requirements or set()
        self.kwargs = kwargs or {}
        self.confidences = confidences or []
        self.use_py3 = use_py3
        self.responsible_users = responsible_users or []
        self.style = style

    def to_mc_format(self):
        result = {
            "name": self.name,
            "type": "mstand",
            "owner": self.owner,
            "greaterBetter": self.greater_better,
            "description": self.description,
            "deprecated": False,
            "style": self.style,
            "responsibleUsers": self.responsible_users,
            "configuration": {
                "revision": self.revision,
                "url": self.url,
                "module": self.metric_module,
                "className": self.metric_class,
                "kwargs": self.kwargs,
                "requirements": sorted(self.requirements),
            },
            "confidences": sorted(self.confidences, key=lambda c: c["name"]),
        }
        if self.use_py3 is not None:
            result["configuration"]["usePy3"] = self.use_py3
        return result

    def copy(self):
        return copy.deepcopy(self)
