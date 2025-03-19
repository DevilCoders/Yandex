"""
Operations metadata
"""
from ...utils.metadata import Metadata, _SubclusterMetadata


class _HadoopSubclusterMetadata(_SubclusterMetadata):
    """
    Subcluster metadata
    """

    def __init__(self, subcid: str) -> None:
        self.subcid = subcid

    def _asdict(self) -> dict:
        return {'subcid': self.subcid}


CreateSubclusterMetadata = _HadoopSubclusterMetadata
ModifySubclusterMetadata = _HadoopSubclusterMetadata
DeleteSubclusterMetadata = _HadoopSubclusterMetadata


class _HadoopJobMetadata(Metadata):
    """
    Job metadata
    """

    def __init__(self, job_id: str) -> None:
        self.job_id = job_id

    def _asdict(self) -> dict:
        return {'jobId': self.job_id}


CreateJobMetadata = _HadoopJobMetadata
