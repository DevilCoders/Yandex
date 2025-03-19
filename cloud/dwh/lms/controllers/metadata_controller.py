from cloud.dwh.lms.services.metadata_service import MetadataService


class MetadataController:
    def __init__(self):
        self._metadata_service = MetadataService()

    def parse_metadata(self, file_path: str):
        return self._metadata_service.parse_metadata(file_path)
