# -*- coding: utf-8 -*-

import experiment_pool.meta as meta


# noinspection PyClassHasNoInit
class TestMeta(object):
    def test_meta_serialize(self):
        meta_object = meta.Meta(
            groups=[
                meta.MetricsGroup(
                    key="mg1",
                    name="гм 1",
                    name_en="mg 1",
                    description="metrics group 1",
                    metrics=[
                        meta.Metric(
                            name="m11", hname="м 11", hname_en="m 11", description="metric 11"
                        ),
                        meta.Metric(
                            name="m12", hname="м 12", hname_en="m 12", description="metric 12"
                        )
                    ]
                ),
                meta.MetricsGroup(
                    key="mg2",
                    metrics=[meta.Metric(name="m21")]
                )
            ],
            additional=meta.Additional(
                pvalue=[
                    meta.AdditionalValue(
                        name="mean", hname="среднее", hname_en="Mean",
                        description="foo", type="scalar"
                    )
                ],
                delta=[meta.AdditionalValue("av1"), meta.AdditionalValue("av2")]
            )
        )

        etalon = {
            "additional": {
                "pvalue": [{
                    "type": "scalar",
                    "description": "foo",
                    "hname": "среднее",
                    "name": "mean",
                    "hname_en": "Mean"
                }],
                "delta": [{"name": "av1"}, {"name": "av2"}]
            },
            "groups": [
                {
                    "metrics": [
                        {
                            "description": "metric 11",
                            "hname": "м 11",
                            "name": "m11",
                            "hname_en": "m 11"
                        },
                        {
                            "description": "metric 12",
                            "hname": "м 12",
                            "name": "m12",
                            "hname_en": "m 12"
                        }
                    ],
                    "name_en": "mg 1",
                    "name": "гм 1",
                    "key": "mg1",
                    "description": "metrics group 1"
                },
                {
                    "metrics":
                    [
                        {
                            "name": "m21"
                        }
                    ],
                    "key": "mg2"
                }
            ],
            "name": ""
        }

        serialized = meta_object.serialize()

        assert etalon == serialized

        assert meta.Meta.deserialize(etalon) == meta_object

        assert meta.Meta.deserialize(etalon).serialize() == etalon == serialized
