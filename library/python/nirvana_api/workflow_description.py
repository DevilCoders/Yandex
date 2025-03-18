class ConnectionInfo:
    def __init__(self, guid=None, source_block_id=None, source_block_code=None, source_endpoint_name=None,
                 dest_endpoint_name=None, dest_block_id=None, dest_block_code=None):
        self.guid = guid
        self.source_block_id = source_block_id
        self.source_block_code = source_block_code
        self.source_endpoint_name = source_endpoint_name
        self.dest_block_id = dest_block_id
        self.dest_block_code = dest_block_code
        self.dest_endpoint_name = dest_endpoint_name

    def __eq_source__(self, other):
        if getattr(other, 'type') == 'operation_connection':
            return ((not self.source_block_id or self.source_block_id == other.sourceBlockId) and
                    (not self.source_block_code or self.source_block_code == other.sourceBlockCode) and
                    (not self.source_endpoint_name or self.source_endpoint_name == other.sourceEndpointName))
        elif getattr(other, 'type') == 'data_connection':
            return ((not self.source_block_id or self.source_block_id == other.sourceDataId) and
                    (not self.source_block_code or self.source_block_code == other.sourceDataCode))
        return False

    def __eq__(self, other):
        return ((not self.guid or self.guid == other.guid) and
                self.__eq_source__(other) and
                (not self.dest_block_id or self.dest_block_id == other.destBlockId) and
                (not self.dest_block_code or self.dest_block_code == other.destBlockCode) and
                (not self.dest_endpoint_name or self.dest_endpoint_name == other.destEndpointName))


class BlockInfo:
    def __init__(self, block_guid=None, block_code=None, type=None, name=None, operation_id=None,
                 stored_data_id=None, storage_path=None):
        self.block_guid = block_guid
        self.block_code = block_code
        self.type = type
        self.name = name
        self.operation_id = operation_id
        self.stored_data_id = stored_data_id
        self.storage_path = storage_path

    def __eq__(self, other):
        return ((not self.block_guid or self.guid == other.blockGuid) and
                (not self.block_code or self.block_code == other.blockCode) and
                (not self.type or self.type == other.type) and
                (not self.name or self.name == other.name) and
                (not self.operation_id or (hasattr(other, 'operationId') and self.operation_id == other.operationId)) and
                (not self.storage_path or (hasattr(other, 'storagePath') and self.storage_path == other.storagePath)) and
                (not self.stored_data_id or (hasattr(other, 'storedDataId') and self.stored_data_id == other.storedDataId)))


class WorkflowDescription:
    def __init__(self, description):
        self.description = description

    def get_connections(self, guid=None, source_block_id=None, source_block_code=None, source_endpoint_name=None,
                        dest_endpoint_name=None, dest_block_id=None, dest_block_code=None):
        connection_info = ConnectionInfo(guid, source_block_id, source_block_code, source_endpoint_name,
                                         dest_endpoint_name, dest_block_id, dest_block_code)
        return [c for c in self.description.connections if c == connection_info]

    def get_blocks(self, block_guid=None, block_code=None, type=None, name=None, operation_id=None,
                   stored_data_id=None, storage_path=None):
        block_info = BlockInfo(block_guid, block_code, type, name, operation_id, stored_data_id, storage_path)
        return [b for b in self.description.blocks if b == block_info]
