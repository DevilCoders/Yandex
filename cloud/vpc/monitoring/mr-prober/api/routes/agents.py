from fastapi import Depends, APIRouter
from sqlalchemy.orm import Session

import settings
from agent.config.s3.uploader import AgentConfigS3Uploader, AgentConfigS3DiffBuilder
from api import models
from api.agents import update_all_clusters_in_s3, update_all_probers_in_s3, uploader, get_diff_builder
from api.dependencies import db

router = APIRouter(
    prefix="/agents",
    tags=["agents"]
)


@router.get(
    "/sync",
    response_model=models.SuccessResponse,
    summary="Sync Agents Configuration into S3",
    description=f"Uploads configuration for agent instances into S3: "
                f"{settings.S3_ENDPOINT}/{settings.AGENT_CONFIGURATIONS_S3_BUCKET}/{settings.S3_PREFIX}",
)
def sync(db: Session = Depends(db), uploader: AgentConfigS3Uploader = Depends(uploader)) -> models.SuccessResponse:
    update_all_clusters_in_s3(db, uploader)
    update_all_probers_in_s3(db, uploader)
    return models.SuccessResponse()


@router.get(
    "/sync/diff",
    response_model=models.AgentsConfigurationSyncDiff,
    summary="View Diff for Agent Configuration Syncing"
)
def get_sync_diff(db: Session = Depends(db)) -> models.AgentsConfigurationSyncDiff:
    diff_builder = get_diff_builder()

    # Do the same things as in `/agents/sync` above, but pass `diff_builder` instead of `uploader`.
    update_all_clusters_in_s3(db, diff_builder)
    update_all_probers_in_s3(db, diff_builder)

    changed_files = diff_builder.get_changed_files()
    return models.AgentsConfigurationSyncDiff(
        unchanged_files=diff_builder.get_unchanged_files(),
        changed_files={
            filename: models.SyncDiffFile(old_content=old, new_content=new) for filename, (old, new) in changed_files.items()
        },
    )
