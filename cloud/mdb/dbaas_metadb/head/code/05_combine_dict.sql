-- Keep in mind that exits plv8 version of that function, which is used in Double Cloud.
-- https://a.yandex-team.ru/arc/trunk/arcadia/cloud/mdb/datacloud/docker/metadb-migrations/resources/code/05_combine_dict.sql?rev=r8969685#L1
CREATE OR REPLACE FUNCTION code.combine_dict(
    i_data jsonb[]
) RETURNS jsonb AS
$$
    import collections.abc
    import json
    def _update(orig, update):
        if orig is None:
            return update
        for key, value in update.items():
            orig_value = orig.get(key, {})
            if isinstance(value, collections.abc.Mapping) and isinstance(orig_value, collections.abc.Mapping):
                orig[key] = _update(orig_value, value)
            else:
                orig[key] = value

        return orig

    _result = {}
    for i in i_data:
        _update(_result, json.loads(i))
    return json.dumps(_result)
$$
LANGUAGE 'plpython3u' IMMUTABLE;
