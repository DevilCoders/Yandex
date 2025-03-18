import heapq


def merge_iterators(iterators, keys):
    prepared = [_prepare_for_merge(iterator, table_index, keys)
                for table_index, iterator in enumerate(iterators)]
    return (row for _, row in heapq.merge(*prepared))


def _prepare_for_merge(rows, table_index, keys):
    previous_key_values = None
    for pos, row in enumerate(rows):
        row["@table_index"] = table_index
        key_values = tuple(row.get(k) for k in keys)
        if previous_key_values is not None:
            assert previous_key_values <= key_values
        previous_key_values = key_values
        yield ((key_values, table_index, pos), row)
