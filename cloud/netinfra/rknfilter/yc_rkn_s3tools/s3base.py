class s3_object_resource:
    def __init__(
        self,
        file_name,
        file_path,
        s3_object_key,
        s3_object_hash,
        transaction_id,
        modification_date,
    ):
        self.file_name = file_name
        self.file_path = file_path
        self.s3_object_key = s3_object_key
        self.s3_object_hash = s3_object_hash
        self.transaction_id = transaction_id
        self.modification_date = modification_date
