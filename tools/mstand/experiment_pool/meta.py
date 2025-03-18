import attr


def coerce_to(type, list=False):
    def _coerce(value):
        if value is None:
            return None

        if isinstance(value, type):
            return value

        if isinstance(value, tuple):
            return type(*value)

        if isinstance(value, dict):
            return type(**value)

        raise ValueError("can't coerce {} to a type {}".format(value, type))

    if list:
        return lambda v: [_coerce(_) for _ in v] if v is not None else None

    return _coerce


class Serializable(object):
    def serialize(self):
        return attr.asdict(self, filter=lambda _, v: v is not None)

    @classmethod
    def deserialize(cls, value):
        return coerce_to(cls)(value)


@attr.s
class Metric(Serializable):
    name = attr.ib()
    hname = attr.ib(default=None)
    hname_en = attr.ib(default=None)
    description = attr.ib(default=None)


@attr.s
class MetricsGroup(Serializable):
    key = attr.ib()
    metrics = attr.ib(converter=coerce_to(Metric, list=True))
    name = attr.ib(default=None)
    name_en = attr.ib(default=None)
    description = attr.ib(default=None)


@attr.s
class AdditionalValue(Serializable):
    name = attr.ib()
    hname = attr.ib(default=None)
    hname_en = attr.ib(default=None)
    description = attr.ib(default=None)
    description_en = attr.ib(default=None)
    type = attr.ib(
        default=None,
        validator=attr.validators.optional(
            attr.validators.in_(["scalar", "ordered_dict", "list", "link", "json"])
        )
    )


@attr.s
class Additional(Serializable):
    pvalue = attr.ib(default=None, converter=coerce_to(AdditionalValue, list=True))
    delta = attr.ib(default=None, converter=coerce_to(AdditionalValue, list=True))
    sigma = attr.ib(default=None, converter=coerce_to(AdditionalValue, list=True))
    delta_percent = attr.ib(default=None, converter=coerce_to(AdditionalValue, list=True))
    value = attr.ib(default=None, converter=coerce_to(AdditionalValue, list=True))


@attr.s
class Meta(Serializable):
    groups = attr.ib(converter=coerce_to(MetricsGroup, list=True))
    additional = attr.ib(default=None, converter=coerce_to(Additional))
    name = attr.ib(default="")
