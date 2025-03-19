"""
Module to provide logs for data proc jobs
"""

import copy
from abc import ABC, abstractmethod
from typing import List, Tuple


class DataprocJobLog(ABC):
    """
    Abstract DataprocJobLog provider
    """

    @abstractmethod
    def get_content(self, cluster_id: str, job_id: str, offset: int, limit: int, **kwargs):
        """
        Return a part of dataproc log
        """


class DataprocJobLogError(Exception):
    """
    Base Dataproc Job Log Error
    """


class DataprocJobLogClientError(DataprocJobLogError):
    """
    Exception for client-side errors
    """


def filter_necessary_parts(sorted_parts: List[dict], offset: int, limit: int) -> Tuple[List[dict], int]:
    """
    Filter only required driveroutput parts for requested offset and limit.
    Return list of parts and nextPageToken for following request
    """
    if offset == 0 and limit == 0:
        return [], 0
    actual_offset = 0
    input_parts = copy.deepcopy(sorted_parts)
    full_length = sum([part['Size'] for part in input_parts])
    parts: list[dict] = []
    page_token = min(offset + limit, full_length)
    prev_part_size = 0
    for part in input_parts:
        # Skip unnecessary parts before requested interval
        if actual_offset + part['Size'] < offset:
            actual_offset += part['Size']
            continue
        # Skip unnecessary parts after requested interval
        # This condition means, that we already added all required pages,
        # so just fix `amount` and break the loop
        if actual_offset >= offset + limit:
            # Truncate unnecessary bytes on last part
            if actual_offset > offset + limit:
                if parts[-1].get('Amount') is None:
                    parts[-1]['Amount'] = offset + limit - actual_offset + prev_part_size
            page_token = offset + limit
            break
        # For first found part we should add seek attribute, for seeking to accurate offset
        if len(parts) == 0:
            seek = offset - actual_offset
            if seek != 0:
                part['Seek'] = seek
        # This condition is quite different, it's means that current part is latest
        # for requested interval
        if actual_offset + part['Size'] > offset + limit:
            # part['Amount'] = offset + limit - actual_offset
            part['Amount'] = offset + limit - actual_offset - part.get('Seek', 0)
            page_token = offset + limit
        actual_offset += part['Size']
        prev_part_size = part['Size']
        # Skip parts, where we should seek whole part
        if part.get('Seek') != part['Size']:
            parts.append(part)
    return parts, page_token
