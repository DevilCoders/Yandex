import os
import logging
from dataclasses import dataclass
from typing import Dict, List, Optional, Any

from yandex_tracker_client import TrackerClient
from yandex_tracker_client.objects import Resource
from clan_tools.utils.timing import timing

logger = logging.getLogger(__name__)


class TrackerException(Exception):
    pass


@dataclass
class UserIssue:
    key: str
    components: List[str]
    summary: str
    status: str
    description: str
    created_at: str
    updated_at: str
    issue: Resource


class TrackerAdapter:
    def __init__(self, token: Optional[str] = None):
        '''Docs https://github.com/yandex/yandex_tracker_client'''
        self._ui_url = 'https://st.yandex-team.ru'
        if token is None:
            token = os.environ['TRACKER_TOKEN']
        self.st_client: TrackerClient = TrackerClient(
            base_url='https://st-api.yandex-team.ru', token=token, org_id='cloud.analytics')

    def get_user_issues(self, user: Optional[str] = None,
                        date_from_updated: Optional[str] = None,
                        date_from_created: Optional[str] = None,
                        queue: str = 'CLOUDANA',
                        components: Optional[List[str]] = None,
                        status: Optional[List[int]] = None,
                        include_description: bool = False) -> List[UserIssue]:
        filter_dict: Dict[str, Any] = {'queue': queue}
        if date_from_updated is not None:
            filter_dict['updated'] = {'from': date_from_updated}

        if date_from_created is not None:
            filter_dict['created'] = {'from': date_from_created}

        if status is not None:
            filter_dict['status'] = status or [4, 19]

        if user is not None:
            filter_dict['assignee'] = user

        if components is not None:
            filter_dict['components'] = components

        issues_resources = self.st_client.issues.find(filter=filter_dict)
        issues: List[UserIssue] = []
        for issue in issues_resources:
            try:
                issues.append(UserIssue(key=issue.key,
                                        components=[
                                            comp.name for comp in issue.components],
                                        summary=issue.summary,
                                        status=issue.status.name,
                                        description=issue.description if include_description else None,
                                        created_at=issue.createdAt,
                                        updated_at=issue.updatedAt,
                                        issue=issue,
                                        ))
            except Exception:
                pass

        return issues

    @timing
    def send_data(self, data: Dict[str, Any]) -> Resource:
        logger.info('Sending data to tracker...')
        response: Resource = self.st_client.issues.create(**data)

        ticket_id = response["self"].split('/')[-1]
        logger.info(f'{self._ui_url}/{ticket_id} is created')

        return response
