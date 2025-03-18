"""
    Some functions to work with shards.
    No guarantees that it works for any shard from tiers.json
"""


def partition_number(shard_template):
    return _shard_numbers(shard_template)[0]


def shard_in_partition_number(shard_template):
    return _shard_numbers(shard_template)[-1]


def _shard_numbers(shard_template):
    """
    Cut off generation.
    Cut all non-numbers from the left (tier name etc.)
    If there are non-numbers between, the function will raise
    """
    shard_template = shard_template.replace('-0000000000', '')
    shard_template = shard_template.replace('-00000000-000000', '')
    parts = shard_template.split('-')
    while parts and not str.isdigit(parts[0]):
        del parts[0]
    numbers = [int(x) for x in parts]
    if len(numbers) not in (1, 2):
        raise ValueError("cannot parse shard name")
    return numbers
