import logging
import re

from reports import ExperimentPair  # noqa


def colorize_experiment_pairs(exp_pairs, value_threshold):
    """
    :type exp_pairs: list[ExperimentPair]
    :type value_threshold: float
    :rtype: None
    """
    logging.info("Setting metric colors")

    for exp_pair in exp_pairs:
        exp_pair.colorize_result_pairs(value_threshold=value_threshold, rows_threshold=0.05)


class GPVerdict(object):
    def __init__(self, bad=0, good=0, source=None):
        """
        :type bad: int
        :type good: int
        :type source: str | None
        """
        assert bad >= 0
        assert good >= 0
        if bad:
            assert not good
        if good:
            assert not bad
        self.bad = bad
        self.good = good
        self.source = source

    def __repr__(self):
        if self.bad:
            return "GPVerdict(bad={!r}, source={!r})".format(self.bad, self.source)
        if self.good:
            return "GPVerdict(good={!r}, source={!r})".format(self.good, self.source)
        return "GPVerdict(source={!r})".format(self.source)

    def __str__(self):
        return repr(self)

    def is_bad(self):
        return bool(self.bad)

    def is_good(self):
        return bool(self.good)

    def is_unknown(self):
        return not self.bad and not self.good


def get_verdict_by_testid(tags, testid):
    if not tags:
        return GPVerdict()

    testid_pattern = "(_{})?".format(testid) if testid else ""
    pattern = re.compile("^(gp_verdict|verdict_bad|verdict_good)_([0-9]+)_[a-z]+{}$".format(testid_pattern))

    verdicts = []
    for tag in tags:
        match = pattern.match(tag)
        if match:
            prefix = match.group(1)
            value = int(match.group(2))
            if prefix == "verdict_good":
                verdict = GPVerdict(good=value, source=tag)
            else:
                verdict = GPVerdict(bad=value, source=tag)
            verdicts.append(verdict)
        if (tag + " ").startswith("green "):
            verdicts.append(GPVerdict(good=1, source=tag))
        if (tag + " ").startswith("red "):
            verdicts.append(GPVerdict(bad=1, source=tag))
        if (tag + " ").startswith("hz "):
            verdicts.append(GPVerdict(good=0, source=tag))

    if not verdicts:
        return GPVerdict()

    if len(verdicts) > 1:
        raise Exception("Duplicate verdicts in testid {}: {}".format(testid, verdicts))
    return verdicts[0]


def get_aspects(tags):
    aspects = None
    if tags is None:
        return None
    for tag in tags:
        match = re.match("gp_aspect_(.+)_(.+)", tag)
        if match:
            if aspects is not None:
                raise Exception("Only one aspect per observation required, found two: {}, gp_aspect_{}_{}".format(tag,
                                                                                                                  aspects[0],
                                                                                                                  aspects[1]))
            aspects = match.group(1), match.group(2)

    return aspects


def format_num(x, prec=7):
    if x is None:
        return "n/a"
    template = "{{:.{:d}g}}".format(prec)
    return template.format(x)
