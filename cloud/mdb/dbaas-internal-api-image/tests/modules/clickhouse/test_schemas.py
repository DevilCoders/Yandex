from typing import Any, Dict, Optional

import pytest
from marshmallow import fields

from dbaas_internal_api.apis.schemas.fields import Environment
from dbaas_internal_api.default_config import ENV_MAPPING
from dbaas_internal_api.modules.clickhouse.schemas.clusters import (
    ClickhouseConfigSchemaV1,
    ClickhouseListClustersResponseSchemaV1,
    ClickhouseCreateClusterRequestSchemaV1,
)
from dbaas_internal_api.modules.clickhouse.schemas.resource_presets import ClickhouseListResourcePresetsSchemaV1
from dbaas_internal_api.modules.clickhouse.schemas.shards import ClickhouseAddShardRequestSchemaV1
from dbaas_internal_api.modules.clickhouse.traits import ClickhouseRoles
from marshmallow.fields import List


def in_dict(data: Optional[Dict[str, Any]] = None) -> Dict[str, Any]:
    """
    Factory function for serialized ClickHouse dictionaries.
    """
    if not data:
        data = {}

    if {'fixedLifetime', 'lifetimeRange'}.isdisjoint(data):
        data['fixedLifetime'] = 300

    source_keys = {'httpSource', 'mysqlSource', 'clickhouseSource', 'mongodbSource'}
    if source_keys.isdisjoint(data):
        data['httpSource'] = {
            'url': 'test_url',
            'format': 'TSV',
        }

    return {
        'name': 'test_dict',
        'layout': {
            'type': 'FLAT',
        },
        'structure': {
            'id': {
                'name': 'id',
            },
            'attributes': [
                {
                    'name': 'text',
                    'type': 'String',
                    'nullValue': '',
                    'hierarchical': False,
                    'injective': False,
                }
            ],
        },
        **data,
    }


def out_dict(data: Optional[Dict[str, Any]] = None) -> Dict[str, Any]:
    """
    Factory function for deserialized ClickHouse dictionaries.
    """
    if not data:
        data = {}

    if {'fixed_lifetime', 'lifetime_range'}.isdisjoint(data):
        data['fixed_lifetime'] = 300

    source_keys = {'http_source', 'mysql_source', 'clickhouse_source', 'mongodb_source'}
    if source_keys.isdisjoint(data):
        data['http_source'] = {
            'url': 'test_url',
            'format': 'TSV',
        }

    return {
        'name': 'test_dict',
        'layout': {
            'type': 'flat',
        },
        'structure': {
            'id': {
                'name': 'id',
            },
            'attributes': [
                {
                    'name': 'text',
                    'type': 'String',
                    'null_value': '',
                    'hierarchical': False,
                    'injective': False,
                }
            ],
        },
        **data,
    }


class TestClickhouseConfigSchemaV1:
    Schema = ClickhouseConfigSchemaV1

    def test_load_empty(self):
        schema = self.Schema()
        data, errors = schema.load({})
        assert not errors
        assert data == {}
        assert not schema.context.get('restart')

    def test_dump_empty(self):
        data, errors = self.Schema().dump({})
        assert not errors
        assert data == {}

    def test_load_log_level(self):
        schema = self.Schema()
        data, errors = schema.load({'logLevel': 'DEBUG'})
        assert not errors
        assert data == {'log_level': 'debug'}
        assert not schema.context.get('restart')

    def test_dump_log_level(self):
        data, errors = self.Schema().dump({'log_level': 'warning'})
        assert not errors
        assert data == {'logLevel': 'WARNING'}

    def test_load_merge_tree(self):
        schema = self.Schema()
        data, errors = schema.load(
            {
                'mergeTree': {
                    'maxReplicatedMergesInQueue': 10,
                },
            }
        )
        assert not errors
        assert data == {
            'merge_tree': {
                'max_replicated_merges_in_queue': 10,
            },
        }
        assert schema.context.get('restart')

    def test_load_dict(self):
        schema = self.Schema()
        data, errors = schema.load(
            {
                'dictionaries': [in_dict()],
            }
        )
        assert not errors
        assert data == {'dictionaries': [out_dict()]}
        assert not schema.context.get('restart')

    def test_dump_dict(self):
        data, errors = self.Schema().dump(
            {
                'dictionaries': [out_dict()],
            }
        )
        assert not errors
        assert data == {
            'dictionaries': [in_dict()],
        }

    def test_load_dict_with_invalid_name_failed(self):
        data, errors = self.Schema().load(
            {
                'dictionaries': [in_dict({'name': 'An invalid name!'})],
            }
        )
        assert errors == {'dictionaries': {0: {'name': ['String does not match expected pattern.']}}}

    def test_load_dict_with_lifetime_range(self):
        data, errors = self.Schema().load(
            {
                'dictionaries': [
                    in_dict(
                        {
                            'lifetimeRange': {
                                'min': 300,
                                'max': 360,
                            },
                        }
                    )
                ],
            }
        )
        assert not errors
        assert data == {
            'dictionaries': [
                out_dict(
                    {
                        'lifetime_range': {
                            'min': 300,
                            'max': 360,
                        },
                    }
                )
            ],
        }

    def test_dump_dict_with_lifetime_range(self):
        data, errors = self.Schema().dump(
            {
                'dictionaries': [
                    out_dict(
                        {
                            'lifetime_range': {
                                'min': 300,
                                'max': 360,
                            },
                        }
                    )
                ],
            }
        )
        assert not errors
        assert data == {
            'dictionaries': [
                in_dict(
                    {
                        'lifetimeRange': {
                            'min': 300,
                            'max': 360,
                        },
                    }
                )
            ],
        }

    def test_load_dict_with_complex_key(self):
        data, errors = self.Schema().load(
            {
                'dictionaries': [
                    in_dict(
                        {
                            'structure': {
                                'key': {
                                    'attributes': [
                                        {
                                            'name': 'key_part_1',
                                            'type': 'String',
                                        },
                                        {
                                            'name': 'key_part_2',
                                            'type': 'String',
                                        },
                                    ],
                                },
                                'attributes': [
                                    {
                                        'name': 'text',
                                        'type': 'String',
                                        'nullValue': '',
                                    }
                                ],
                            },
                            'layout': {
                                'type': 'COMPLEX_KEY_HASHED',
                            },
                        }
                    )
                ],
            }
        )
        assert not errors
        assert data == {
            'dictionaries': [
                out_dict(
                    {
                        'structure': {
                            'key': {
                                'attributes': [
                                    {
                                        'name': 'key_part_1',
                                        'type': 'String',
                                        'null_value': '',
                                        'hierarchical': False,
                                        'injective': False,
                                    },
                                    {
                                        'name': 'key_part_2',
                                        'type': 'String',
                                        'null_value': '',
                                        'hierarchical': False,
                                        'injective': False,
                                    },
                                ],
                            },
                            'attributes': [
                                {
                                    'name': 'text',
                                    'type': 'String',
                                    'null_value': '',
                                    'hierarchical': False,
                                    'injective': False,
                                }
                            ],
                        },
                        'layout': {
                            'type': 'complex_key_hashed',
                        },
                    }
                )
            ],
        }

    def test_dump_dict_with_complex_key(self):
        data, errors = self.Schema().dump(
            {
                'dictionaries': [
                    out_dict(
                        {
                            'structure': {
                                'key': {
                                    'attributes': [
                                        {
                                            'name': 'key_part_1',
                                            'type': 'String',
                                        },
                                        {
                                            'name': 'key_part_2',
                                            'type': 'String',
                                        },
                                    ],
                                },
                                'attributes': [
                                    {
                                        'name': 'text',
                                        'type': 'String',
                                        'null_value': '',
                                    }
                                ],
                            },
                            'layout': {
                                'type': 'complex_key_hashed',
                            },
                        }
                    )
                ],
            }
        )
        assert not errors
        assert data == {
            'dictionaries': [
                in_dict(
                    {
                        'structure': {
                            'key': {
                                'attributes': [
                                    {
                                        'name': 'key_part_1',
                                        'type': 'String',
                                    },
                                    {
                                        'name': 'key_part_2',
                                        'type': 'String',
                                    },
                                ],
                            },
                            'attributes': [
                                {
                                    'name': 'text',
                                    'type': 'String',
                                    'nullValue': '',
                                }
                            ],
                        },
                        'layout': {
                            'type': 'COMPLEX_KEY_HASHED',
                        },
                    }
                )
            ],
        }

    def test_load_dict_with_size_in_cells(self):
        data, errors = self.Schema().load(
            {
                'dictionaries': [
                    in_dict(
                        {
                            'layout': {
                                'type': 'CACHE',
                                'sizeInCells': 1000000000,
                            },
                        }
                    )
                ],
            }
        )
        assert not errors
        assert data == {
            'dictionaries': [
                out_dict(
                    {
                        'layout': {
                            'type': 'cache',
                            'size_in_cells': 1000000000,
                        },
                    }
                )
            ],
        }

    def test_dump_dict_with_size_in_cells(self):
        data, errors = self.Schema().dump(
            {
                'dictionaries': [
                    out_dict(
                        {
                            'layout': {
                                'type': 'cache',
                                'size_in_cells': 1000000000,
                            },
                        }
                    )
                ],
            }
        )
        assert not errors
        assert data == {
            'dictionaries': [
                in_dict(
                    {
                        'layout': {
                            'type': 'CACHE',
                            'sizeInCells': 1000000000,
                        },
                    }
                )
            ],
        }

    def test_load_dict_with_no_attributes_failed(self):
        data, errors = self.Schema().load(
            {
                'dictionaries': [
                    in_dict(
                        {
                            'structure': {
                                'id': {
                                    'name': 'id',
                                },
                            },
                        }
                    )
                ],
            }
        )
        assert errors == {
            'dictionaries': {
                0: {
                    'structure': {
                        'attributes': {
                            0: {
                                'name': ['Missing data for required field.'],
                                'type': ['Missing data for required field.'],
                            },
                        },
                    },
                },
            },
        }

    def test_load_dict_with_mysql_source(self):
        data, errors = self.Schema().load(
            {
                'dictionaries': [
                    in_dict(
                        {
                            'mysqlSource': {
                                'db': 'test_db',
                                'table': 'test_collection',
                                'replicas': [{'host': 'test_host'}, {'host': 'test_host2', 'priority': 50}],
                                'where': 'a > b',
                            },
                        }
                    )
                ],
            }
        )
        assert not errors
        assert data == {
            'dictionaries': [
                out_dict(
                    {
                        'mysql_source': {
                            'db': 'test_db',
                            'table': 'test_collection',
                            'replicas': [
                                {'host': 'test_host', 'priority': 100},
                                {'host': 'test_host2', 'priority': 50},
                            ],
                            'where': 'a &gt; b',
                        },
                    }
                )
            ],
        }

    def test_dump_dict_with_mysql_source(self):
        data, errors = self.Schema().dump(
            {
                'dictionaries': [
                    out_dict(
                        {
                            'mysql_source': {
                                'type': 'mysql',
                                'db': 'test_db',
                                'table': 'test_collection',
                                'replicas': [
                                    {'host': 'test_host'},
                                ],
                                'where': 'a &gt; b',
                            },
                        }
                    )
                ],
            }
        )
        assert not errors
        assert data == {
            'dictionaries': [
                in_dict(
                    {
                        'mysqlSource': {
                            'db': 'test_db',
                            'table': 'test_collection',
                            'replicas': [
                                {'host': 'test_host'},
                            ],
                            'where': 'a > b',
                        },
                    }
                )
            ],
        }

    def test_load_dict_with_clickhouse_source(self):
        data, errors = self.Schema().load(
            {
                'dictionaries': [
                    in_dict(
                        {
                            'clickhouseSource': {
                                'host': 'test_host',
                                'port': 9000,
                                'db': 'test_db',
                                'table': 'test_table',
                            },
                        }
                    )
                ],
            }
        )
        assert not errors
        assert data == {
            'dictionaries': [
                out_dict(
                    {
                        'clickhouse_source': {
                            'host': 'test_host',
                            'port': 9000,
                            'db': 'test_db',
                            'table': 'test_table',
                        },
                    }
                )
            ],
        }

    def test_dump_dict_with_clickhouse_source(self):
        data, errors = self.Schema().dump(
            {
                'dictionaries': [
                    out_dict(
                        {
                            'clickhouse_source': {
                                'type': 'clickhouse',
                                'host': 'test_host',
                                'port': 9000,
                                'db': 'test_db',
                                'table': 'test_table',
                            },
                        }
                    )
                ],
            }
        )
        assert not errors
        assert data == {
            'dictionaries': [
                in_dict(
                    {
                        'clickhouseSource': {
                            'host': 'test_host',
                            'port': 9000,
                            'db': 'test_db',
                            'table': 'test_table',
                        },
                    }
                )
            ],
        }

    def test_load_dict_with_mongodb_source(self):
        data, errors = self.Schema().load(
            {
                'dictionaries': [
                    in_dict(
                        {
                            'mongodbSource': {
                                'host': 'test_host',
                                'port': 27018,
                                'db': 'test_db',
                                'collection': 'test_collection',
                            },
                        }
                    )
                ],
            }
        )
        assert not errors
        assert data == {
            'dictionaries': [
                out_dict(
                    {
                        'mongodb_source': {
                            'host': 'test_host',
                            'port': 27018,
                            'db': 'test_db',
                            'collection': 'test_collection',
                        },
                    }
                )
            ],
        }

    def test_dump_dict_with_mongodb_source(self):
        data, errors = self.Schema().dump(
            {
                'dictionaries': [
                    out_dict(
                        {
                            'mongodb_source': {
                                'host': 'test_host',
                                'port': 27018,
                                'db': 'test_db',
                                'collection': 'test_collection',
                            },
                        }
                    )
                ],
            }
        )
        assert not errors
        assert data == {
            'dictionaries': [
                in_dict(
                    {
                        'mongodbSource': {
                            'host': 'test_host',
                            'port': 27018,
                            'db': 'test_db',
                            'collection': 'test_collection',
                        },
                    }
                )
            ],
        }

    def test_load_multiple_dicts(self):
        data, errors = self.Schema().load(
            {
                'dictionaries': [
                    in_dict(
                        {
                            'name': 'test_dict_1',
                            'structure': {
                                'id': {
                                    'name': 'id',
                                },
                                'attributes': [
                                    {
                                        'name': 'text',
                                        'type': 'String',
                                        'nullValue': '',
                                    }
                                ],
                            },
                            'layout': {
                                'type': 'CACHE',
                                'sizeInCells': 1000000000,
                            },
                        }
                    ),
                    in_dict(
                        {
                            'name': 'test_dict_2',
                            'structure': {
                                'key': {
                                    'attributes': [
                                        {
                                            'name': 'key_part_1',
                                            'type': 'String',
                                        },
                                        {
                                            'name': 'key_part_2',
                                            'type': 'String',
                                        },
                                    ],
                                },
                                'attributes': [
                                    {
                                        'name': 'text',
                                        'type': 'String',
                                        'nullValue': '',
                                    }
                                ],
                            },
                            'layout': {
                                'type': 'COMPLEX_KEY_HASHED',
                            },
                        }
                    ),
                ],
            }
        )
        assert not errors
        res = {
            'dictionaries': [
                out_dict(
                    {
                        'name': 'test_dict_1',
                        'structure': {
                            'id': {
                                'name': 'id',
                            },
                            'attributes': [
                                {
                                    'name': 'text',
                                    'type': 'String',
                                    'null_value': '',
                                    'hierarchical': False,
                                    'injective': False,
                                }
                            ],
                        },
                        'layout': {
                            'type': 'cache',
                            'size_in_cells': 1000000000,
                        },
                    }
                ),
                out_dict(
                    {
                        'name': 'test_dict_2',
                        'structure': {
                            'key': {
                                'attributes': [
                                    {
                                        'name': 'key_part_1',
                                        'type': 'String',
                                        'null_value': '',
                                        'hierarchical': False,
                                        'injective': False,
                                    },
                                    {
                                        'name': 'key_part_2',
                                        'type': 'String',
                                        'null_value': '',
                                        'hierarchical': False,
                                        'injective': False,
                                    },
                                ],
                            },
                            'attributes': [
                                {
                                    'name': 'text',
                                    'type': 'String',
                                    'null_value': '',
                                    'hierarchical': False,
                                    'injective': False,
                                }
                            ],
                        },
                        'layout': {
                            'type': 'complex_key_hashed',
                        },
                    }
                ),
            ],
        }

        assert data == res, f'{data} != {res}'

    def test_load_with_hashed_layout_and_zero_size_in_cells(self):
        data, errors = self.Schema().load(
            {
                'dictionaries': [
                    in_dict(
                        {
                            'name': 'test_dict_1',
                            'structure': {
                                'key': {
                                    'attributes': [
                                        {
                                            'name': 'key_part_1',
                                            'type': 'String',
                                        },
                                        {
                                            'name': 'key_part_2',
                                            'type': 'String',
                                        },
                                    ],
                                },
                                'attributes': [
                                    {
                                        'name': 'text',
                                        'type': 'String',
                                        'nullValue': '',
                                    }
                                ],
                            },
                            'layout': {
                                'type': 'COMPLEX_KEY_HASHED',
                                'sizeInCells': 0,
                            },
                        }
                    ),
                    in_dict(
                        {
                            'name': 'test_dict_2',
                            'structure': {
                                'key': {
                                    'attributes': [
                                        {
                                            'name': 'key_part_1',
                                            'type': 'String',
                                        },
                                        {
                                            'name': 'key_part_2',
                                            'type': 'String',
                                        },
                                    ],
                                },
                                'attributes': [
                                    {
                                        'name': 'text',
                                        'type': 'String',
                                        'nullValue': '',
                                    }
                                ],
                            },
                            'layout': {
                                'type': 'COMPLEX_KEY_HASHED',
                                'sizeInCells': '0',
                            },
                        }
                    ),
                ],
            }
        )
        assert not errors
        assert data == {
            'dictionaries': [
                out_dict(
                    {
                        'name': 'test_dict_1',
                        'structure': {
                            'key': {
                                'attributes': [
                                    {
                                        'name': 'key_part_1',
                                        'type': 'String',
                                        'null_value': '',
                                        'hierarchical': False,
                                        'injective': False,
                                    },
                                    {
                                        'name': 'key_part_2',
                                        'type': 'String',
                                        'null_value': '',
                                        'hierarchical': False,
                                        'injective': False,
                                    },
                                ],
                            },
                            'attributes': [
                                {
                                    'name': 'text',
                                    'type': 'String',
                                    'null_value': '',
                                    'hierarchical': False,
                                    'injective': False,
                                }
                            ],
                        },
                        'layout': {
                            'type': 'complex_key_hashed',
                        },
                    }
                ),
                out_dict(
                    {
                        'name': 'test_dict_2',
                        'structure': {
                            'key': {
                                'attributes': [
                                    {
                                        'name': 'key_part_1',
                                        'type': 'String',
                                        'null_value': '',
                                        'hierarchical': False,
                                        'injective': False,
                                    },
                                    {
                                        'name': 'key_part_2',
                                        'type': 'String',
                                        'null_value': '',
                                        'hierarchical': False,
                                        'injective': False,
                                    },
                                ],
                            },
                            'attributes': [
                                {
                                    'name': 'text',
                                    'type': 'String',
                                    'null_value': '',
                                    'hierarchical': False,
                                    'injective': False,
                                }
                            ],
                        },
                        'layout': {
                            'type': 'complex_key_hashed',
                        },
                    }
                ),
            ],
        }

    @pytest.mark.parametrize(
        'layout',
        [
            {
                'type': 'COMPLEX_KEY_CACHE',
            },
            {
                'type': 'COMPLEX_KEY_CACHE',
                'sizeInCells': '0',
            },
            {
                'type': 'COMPLEX_KEY_CACHE',
                'sizeInCells': 0,
            },
        ],
    )
    def test_load_with_cached_layout_and_no_size_in_cells(self, layout):
        data, errors = self.Schema().load(
            {
                'dictionaries': [
                    in_dict(
                        {
                            'name': 'test_dict_1',
                            'structure': {
                                'key': {
                                    'attributes': [
                                        {
                                            'name': 'key_part_1',
                                            'type': 'String',
                                        },
                                        {
                                            'name': 'key_part_2',
                                            'type': 'String',
                                        },
                                    ],
                                },
                                'attributes': [
                                    {
                                        'name': 'text',
                                        'type': 'String',
                                        'nullValue': '',
                                    }
                                ],
                            },
                            'layout': layout,
                        }
                    ),
                ],
            }
        )
        assert errors == {
            'dictionaries': {
                0: {
                    'layout': {
                        '_schema': ["The parameter 'sizeInCells' is required for the layout 'COMPLEX_KEY_CACHE'."],
                    },
                },
            },
        }

    def test_load_with_range_hashed_layout(self):
        data, errors = self.Schema().load(
            {
                'dictionaries': [
                    in_dict(
                        {
                            'structure': {
                                'id': {
                                    'name': 'id',
                                },
                                'rangeMin': {
                                    'name': 'min',
                                },
                                'rangeMax': {
                                    'name': 'max',
                                },
                                'attributes': [
                                    {
                                        'name': 'text',
                                        'type': 'String',
                                        'nullValue': '',
                                    }
                                ],
                            },
                            'layout': {
                                'type': 'RANGE_HASHED',
                            },
                        }
                    ),
                ],
            }
        )
        assert not errors
        assert data == {
            'dictionaries': [
                out_dict(
                    {
                        'structure': {
                            'id': {
                                'name': 'id',
                            },
                            'range_min': {
                                'name': 'min',
                            },
                            'range_max': {
                                'name': 'max',
                            },
                            'attributes': [
                                {
                                    'name': 'text',
                                    'type': 'String',
                                    'null_value': '',
                                    'hierarchical': False,
                                    'injective': False,
                                }
                            ],
                        },
                        'layout': {
                            'type': 'range_hashed',
                        },
                    }
                ),
            ],
        }

    def test_load_with_range_hashed_layout_and_without_range_min(self):
        data, errors = self.Schema().load(
            {
                'dictionaries': [
                    in_dict(
                        {
                            'structure': {
                                'id': {
                                    'name': 'id',
                                },
                                'rangeMax': {
                                    'name': 'max',
                                },
                                'attributes': [
                                    {
                                        'name': 'text',
                                        'type': 'String',
                                        'nullValue': '',
                                    }
                                ],
                            },
                            'layout': {
                                'type': 'RANGE_HASHED',
                            },
                        }
                    ),
                ],
            }
        )
        assert errors == {
            'dictionaries': {
                0: {
                    '_schema': ["Range min must be declared for dictionaries with range hashed layout."],
                },
            },
        }

    def test_load_with_range_hashed_layout_and_without_range_max(self):
        data, errors = self.Schema().load(
            {
                'dictionaries': [
                    in_dict(
                        {
                            'structure': {
                                'id': {
                                    'name': 'id',
                                },
                                'rangeMin': {
                                    'name': 'min',
                                },
                                'attributes': [
                                    {
                                        'name': 'text',
                                        'type': 'String',
                                        'nullValue': '',
                                    }
                                ],
                            },
                            'layout': {
                                'type': 'RANGE_HASHED',
                            },
                        }
                    ),
                ],
            }
        )
        assert errors == {
            'dictionaries': {
                0: {
                    '_schema': ["Range max must be declared for dictionaries with range hashed layout."],
                },
            },
        }

    def test_load_with_cache_layout_and_range_min_attribute(self):
        data, errors = self.Schema().load(
            {
                'dictionaries': [
                    in_dict(
                        {
                            'structure': {
                                'id': {
                                    'name': 'id',
                                },
                                'rangeMin': {
                                    'name': 'min',
                                },
                                'attributes': [
                                    {
                                        'name': 'text',
                                        'type': 'String',
                                        'nullValue': '',
                                    }
                                ],
                            },
                            'layout': {
                                'type': 'FLAT',
                            },
                        }
                    ),
                ],
            }
        )
        assert errors == {
            'dictionaries': {
                0: {
                    '_schema': ["Range min must not be declared for dictionaries with layout other than range hashed."],
                },
            },
        }

    def test_load_with_cache_layout_and_range_max_attribute(self):
        data, errors = self.Schema().load(
            {
                'dictionaries': [
                    in_dict(
                        {
                            'structure': {
                                'id': {
                                    'name': 'id',
                                },
                                'rangeMax': {
                                    'name': 'max',
                                },
                                'attributes': [
                                    {
                                        'name': 'text',
                                        'type': 'String',
                                        'nullValue': '',
                                    }
                                ],
                            },
                            'layout': {
                                'type': 'FLAT',
                            },
                        }
                    ),
                ],
            }
        )
        assert errors == {
            'dictionaries': {
                0: {
                    '_schema': ["Range max must not be declared for dictionaries with layout other than range hashed."],
                },
            },
        }


class TestClickhouseListClustersResponseSchemaV1:
    Schema = ClickhouseListClustersResponseSchemaV1

    def test_load_empty_failed(self):
        data, errors = self.Schema().load({})
        assert errors

    def test_dump_empty_succeeded(self):
        data, errors = self.Schema().dump({})
        assert not errors
        assert data == {}

    def test_load_0_clusters_succeeded(self):
        data, errors = self.Schema().load({'clusters': []})
        assert not errors
        assert data == {'clusters': []}

    def test_dump_0_clusters_succeeded(self):
        data, errors = self.Schema().dump({'clusters': []})
        assert not errors
        assert data == {'clusters': []}

    def test_load_invalid_cluster_failed(self):
        data, errors = self.Schema().load(
            {
                'clusters': [
                    {
                        'id': 'test_id',
                        'name': 'test_name',
                        'config': {
                            'clickhouse': {
                                'config': {
                                    'effectiveConfig': {
                                        'logLevel': 'INVALID',
                                        'maxConnections': 10,
                                    },
                                },
                            },
                        },
                    },
                ]
            }
        )
        assert errors
        assert data == {
            'clusters': [
                {
                    'id': 'test_id',
                    'name': 'test_name',
                    'config': {
                        'clickhouse': {
                            'config': {
                                'effective_config': {
                                    'max_connections': 10,
                                },
                            },
                        },
                    },
                },
            ]
        }

    def test_dump_invalid_cluster_failed(self):
        data, errors = self.Schema().dump(
            {
                'clusters': [
                    {
                        'id': 'test_id',
                        'name': 'test_name',
                        'config': {
                            'clickhouse': {
                                'config': {
                                    'effective_config': {
                                        'log_level': 'invalid',
                                        'max_connections': 10,
                                    },
                                },
                            },
                        },
                    },
                ]
            }
        )
        assert errors
        assert data == {
            'clusters': [
                {
                    'id': 'test_id',
                    'name': 'test_name',
                    'deletionProtection': False,
                    'config': {
                        'clickhouse': {
                            'config': {
                                'effectiveConfig': {
                                    'maxConnections': 10,
                                },
                            },
                        },
                    },
                },
            ]
        }


class TestClickhouseListResourcePresetsSchemaV1:
    Schema = ClickhouseListResourcePresetsSchemaV1

    def test_dump_empty_succeeded(self):
        data, errors = self.Schema().dump({})
        assert not errors
        assert data == {}

    def test_dump_0_objects_succeeded(self):
        data, errors = self.Schema().dump({'resource_presets': []})
        assert not errors
        assert data == {'resourcePresets': []}

    def test_dump_with_enum_roles_succeeded(self):
        data, errors = self.Schema().dump(
            {
                'resource_presets': [
                    {
                        'id': 'test_id',
                        'cores': 1,
                        'core_fraction': 100,
                        'memory': 1,
                        'zone_ids': ['zone_1', 'zone_2'],
                        'roles': [ClickhouseRoles.zookeeper, ClickhouseRoles.clickhouse],
                    },
                ]
            }
        )
        assert not errors
        assert data == {
            'resourcePresets': [
                {
                    'id': 'test_id',
                    'cores': 1,
                    'coreFraction': 100,
                    'memory': 1,
                    'zoneIds': ['zone_1', 'zone_2'],
                    'hostTypes': ['ZOOKEEPER', 'CLICKHOUSE'],
                },
            ]
        }

    def test_dump_with_str_roles_succeeded(self):
        data, errors = self.Schema().dump(
            {
                'resource_presets': [
                    {
                        'id': 'test_id',
                        'cores': 1,
                        'core_fraction': 100,
                        'memory': 1,
                        'zone_ids': ['zone_1', 'zone_2'],
                        'roles': ['zk', 'clickhouse_cluster'],
                    },
                ]
            }
        )
        assert not errors
        assert data == {
            'resourcePresets': [
                {
                    'id': 'test_id',
                    'cores': 1,
                    'coreFraction': 100,
                    'memory': 1,
                    'zoneIds': ['zone_1', 'zone_2'],
                    'hostTypes': ['ZOOKEEPER', 'CLICKHOUSE'],
                },
            ]
        }


class TestClickhouseAddShardRequestSchemaV1:
    Schema = ClickhouseAddShardRequestSchemaV1

    def test_load_request_with_grpc_defaults(self):
        data, errors = self.Schema().load(
            {
                'shardName': 'shard1',
                'configSpec': {
                    'clickhouse': {
                        'config': {
                            'compression': [],
                            'dictionaries': [],
                            'graphiteRollup': [],
                        },
                        'resources': {'diskSize': '0'},
                    },
                },
                'hostSpecs': [],
            }
        )
        assert not errors
        assert data == {
            'shard_name': 'shard1',
            'config_spec': {
                'clickhouse': {
                    'config': {},
                    'resources': {},
                },
            },
            'host_specs': [],
            'copy_schema': False,
        }


class TestClickhouseCreateClusterRequestSchemaV1:
    class Schema(ClickhouseCreateClusterRequestSchemaV1):
        """
        Customized schema for create cluster request that does not access flask application context.
        """

        environment = Environment(required=True, mapping=lambda: ENV_MAPPING)
        hostSpecs = List(fields.Dict)

    @pytest.mark.parametrize(
        ('input', 'result'),
        [
            # request with required parameters only
            [
                {
                    'folderId': 'folder1',
                    'name': 'test_cluster',
                    'environment': 'PRESTABLE',
                    'configSpec': {
                        'clickhouse': {
                            'resources': {
                                'resourcePresetId': 's1.compute.1',
                                'diskTypeId': 'network-ssd',
                                'diskSize': 10737418240,
                            }
                        }
                    },
                    'databaseSpecs': [{'name': 'testdb'}],
                    'userSpecs': [{'name': 'test', 'password': 'test_password'}],
                    'networkId': 'network1',
                },
                {
                    'folder_id': 'folder1',
                    'name': 'test_cluster',
                    'environment': 'qa',
                    'config_spec': {
                        'clickhouse': {
                            'resources': {
                                'resource_preset_id': 's1.compute.1',
                                'disk_type_id': 'network-ssd',
                                'disk_size': 10737418240,
                            },
                        },
                    },
                    'database_specs': [
                        {
                            'name': 'testdb',
                        }
                    ],
                    'user_specs': [
                        {
                            'name': 'test',
                            'password': 'test_password',
                        }
                    ],
                    'network_id': 'network1',
                },
            ],
            # request with optional parameters
            [
                {
                    'folderId': 'folder1',
                    'name': 'test_cluster',
                    'description': 'test cluster',
                    'environment': 'PRESTABLE',
                    'configSpec': {
                        'clickhouse': {
                            'resources': {
                                'resourcePresetId': 's1.compute.1',
                                'diskTypeId': 'network-ssd',
                                'diskSize': 10737418240,
                            }
                        }
                    },
                    'databaseSpecs': [{'name': 'testdb'}],
                    'userSpecs': [{'name': 'test', 'password': 'test_password'}],
                    'networkId': 'network1',
                    'shardName': 'shard1',
                    'serviceAccountId': 'sa1',
                },
                {
                    'folder_id': 'folder1',
                    'name': 'test_cluster',
                    'description': 'test cluster',
                    'environment': 'qa',
                    'config_spec': {
                        'clickhouse': {
                            'resources': {
                                'resource_preset_id': 's1.compute.1',
                                'disk_type_id': 'network-ssd',
                                'disk_size': 10737418240,
                            },
                        },
                    },
                    'database_specs': [
                        {
                            'name': 'testdb',
                        }
                    ],
                    'user_specs': [
                        {
                            'name': 'test',
                            'password': 'test_password',
                        }
                    ],
                    'network_id': 'network1',
                    'shard_name': 'shard1',
                    'service_account_id': 'sa1',
                },
            ],
            # request with explicit nulls
            [
                {
                    'folderId': 'folder1',
                    'name': 'test_cluster',
                    'environment': 'PRESTABLE',
                    'configSpec': {
                        'clickhouse': {
                            'resources': {
                                'resourcePresetId': 's1.compute.1',
                                'diskTypeId': 'network-ssd',
                                'diskSize': 10737418240,
                            }
                        }
                    },
                    'databaseSpecs': [{'name': 'testdb'}],
                    'userSpecs': [{'name': 'test', 'password': 'test_password'}],
                    'networkId': 'network1',
                    'serviceAccountId': None,
                    'shardName': None,
                },
                {
                    'folder_id': 'folder1',
                    'name': 'test_cluster',
                    'environment': 'qa',
                    'config_spec': {
                        'clickhouse': {
                            'resources': {
                                'resource_preset_id': 's1.compute.1',
                                'disk_type_id': 'network-ssd',
                                'disk_size': 10737418240,
                            },
                        },
                    },
                    'database_specs': [
                        {
                            'name': 'testdb',
                        }
                    ],
                    'user_specs': [
                        {
                            'name': 'test',
                            'password': 'test_password',
                        }
                    ],
                    'network_id': 'network1',
                    'service_account_id': None,
                    'shard_name': None,
                },
            ],
            # request with special symbols in names and identifiers
            [
                {
                    'folderId': 'folder1',
                    'name': 'cluster-1aA',
                    'environment': 'PRESTABLE',
                    'configSpec': {
                        'clickhouse': {
                            'resources': {
                                'resourcePresetId': 's1.compute.1',
                                'diskTypeId': 'network-ssd',
                                'diskSize': 10737418240,
                            }
                        }
                    },
                    'databaseSpecs': [{'name': 'db-1aA'}],
                    'userSpecs': [{'name': 'user-1aA', 'password': 'test_password'}],
                    'networkId': 'network1',
                    'shardName': 'shard-1aA',
                    'serviceAccountId': 'sa-1aA_.',
                },
                {
                    'folder_id': 'folder1',
                    'name': 'cluster-1aA',
                    'environment': 'qa',
                    'config_spec': {
                        'clickhouse': {
                            'resources': {
                                'resource_preset_id': 's1.compute.1',
                                'disk_type_id': 'network-ssd',
                                'disk_size': 10737418240,
                            },
                        },
                    },
                    'database_specs': [{'name': 'db-1aA'}],
                    'user_specs': [
                        {
                            'name': 'user-1aA',
                            'password': 'test_password',
                        }
                    ],
                    'network_id': 'network1',
                    'shard_name': 'shard-1aA',
                    'service_account_id': 'sa-1aA_.',
                },
            ],
            # request with cloud storage
            [
                {
                    'folderId': 'folder1',
                    'name': 'test_cluster',
                    'environment': 'PRESTABLE',
                    'configSpec': {
                        'clickhouse': {
                            'resources': {
                                'resourcePresetId': 's1.compute.1',
                                'diskTypeId': 'network-ssd',
                                'diskSize': 10737418240,
                            }
                        },
                        'cloudStorage': {
                            'enabled': True,
                            'dataCacheEnabled': True,
                            'dataCacheMaxSize': 1024,
                            'moveFactor': 0.1,
                        },
                    },
                    'databaseSpecs': [{'name': 'testdb'}],
                    'userSpecs': [{'name': 'test', 'password': 'test_password'}],
                    'networkId': 'network1',
                },
                {
                    'folder_id': 'folder1',
                    'name': 'test_cluster',
                    'environment': 'qa',
                    'config_spec': {
                        'clickhouse': {
                            'resources': {
                                'resource_preset_id': 's1.compute.1',
                                'disk_type_id': 'network-ssd',
                                'disk_size': 10737418240,
                            },
                        },
                        'cloud_storage': {
                            'enabled': True,
                            'data_cache_enabled': True,
                            'data_cache_max_size': 1024,
                            'move_factor': 0.1,
                        },
                    },
                    'database_specs': [
                        {
                            'name': 'testdb',
                        }
                    ],
                    'user_specs': [
                        {
                            'name': 'test',
                            'password': 'test_password',
                        }
                    ],
                    'network_id': 'network1',
                },
            ],
        ],
    )
    def test_load_request(self, input, result):
        actual_result, errors = self.Schema().load(input)
        assert not errors
        assert actual_result == result
