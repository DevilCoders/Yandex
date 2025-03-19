import json

"""
    Client lib for the private NBS API
"""


def describe_volume(client, disk_id):
    input_bytes = '''{{
        "DiskId": "{}"
    }}'''.format(disk_id)

    try:
        output = client.execute_action(action="DescribeVolume", input_bytes=input_bytes)
    except Exception as e:
        notFoundError = "SEVERITY_ERROR FACILITY_SCHEMESHARD status:2 Path not found"
        if str(e).find(notFoundError) != -1:
            return None
        raise

    description = json.loads(output)

    config = description.get('VolumeConfig', {})

    if ('Partitions' in description or 'Partitions' in config) and \
            'VolumeTabletId' in description:
        return description

    raise Exception('Not a BlockStoreVolume description')


def describe_blocks(client, disk_id, start_index, blocks_count, checkpoint_id=""):
    input_bytes = '''{{
        "DiskId": "{}",
        "StartIndex": {},
        "BlocksCount": {},
        "CheckpointId": "{}"
    }}'''.format(disk_id, int(start_index), int(blocks_count), checkpoint_id)

    output = client.execute_action(action="DescribeBlocks", input_bytes=input_bytes)
    return json.loads(output)


def check_blob(client, blob_id, bs_group_id, index_only):
    index_only_string = 'true' if index_only else 'false'
    input_bytes = '''{{
        "BlobId": {},
        "BSGroupId": {},
        "IndexOnly": {}
    }}'''.format(json.dumps(blob_id), int(bs_group_id), index_only_string)

    output = client.execute_action(action="CheckBlob", input_bytes=input_bytes)
    j = json.loads(output)
    if 'Status' not in j:
        raise Exception('CheckBlob response should contain Status field')

    return j


def reset_tablet(client, tablet_id, gen):
    input_bytes = '''{{
        "TabletId": {},
        "Generation": {}
    }}'''.format(int(tablet_id), int(gen))

    output = client.execute_action(action="ResetTablet", input_bytes=input_bytes)
    j = json.loads(output)
    if 'Status' not in j:
        raise Exception('ResetTablet response should contain Status field')

    return j


def compact_disk(client, disk_id, start_index, blocks_count):
    input_bytes = '''{{
        "DiskId": "{}",
        "StartIndex": {},
        "BlocksCount": {}
    }}'''.format(disk_id, int(start_index), int(blocks_count))

    output = client.execute_action(action="CompactRange", input_bytes=input_bytes)
    return json.loads(output)


def get_compaction_status(client, disk_id, operation_id):
    input_bytes = '''{{
        "DiskId": "{}",
        "OperationId": "{}"
    }}'''.format(disk_id, operation_id)

    output = client.execute_action(action="GetCompactionStatus", input_bytes=input_bytes)
    return json.loads(output)
