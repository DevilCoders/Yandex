import json
from collections import namedtuple
from datetime import datetime


from cloud.ai.nirvana.nv_launcher_agent.lib.config import Config
from cloud.ai.nirvana.nv_launcher_agent.lib.thread_logger import ThreadLogger

ReleaseInfo = namedtuple(
    'ReleaseInfo',
    field_names=[
        "release_name",
        "revision_id",
        "resource_name",
        "s3_path",
        "info",
        "timestamp"
    ]
)


class ReleaseManager:
    def __init__(self, release_info_file):
        with open(release_info_file, 'r') as f:
            obj = json.load(f)

        self.release_info_file = release_info_file
        self.releases = {}
        for key, value in obj.items():
            self.releases[key] = ReleaseInfo(**value)

    def get_latest(self) -> ReleaseInfo:
        return sorted(self.releases.values(), key=lambda x: x.timestamp)[-1]

    def update_release_info_from_json(self, release_info: ReleaseInfo):
        ThreadLogger.info(
            f'Update release info from json: '
            f'release_info = {str(release_info)}'
        )
        self.releases[release_info.release_name] = release_info
        with open(self.release_info_file, 'w') as f:
            json.dump({key: value._asdict() for key, value in self.releases.items()}, f, indent=4)

    def update_release_info(self, release_name, resource_name, revision, info):
        ThreadLogger.info(
            f'Update release info: '
            f'release_name={release_name}, resource_name={resource_name}, revision={revision}, info={info}'
        )
        if release_name in self.releases:
            raise ValueError(f"resource_name={resource_name} already in release_info")

        self.releases[release_name] = ReleaseInfo(
            release_name=release_name,
            revision_id=revision,
            resource_name=resource_name,
            s3_path=Config.get_release_s3_path(resource_name),
            info=info,
            timestamp=str(datetime.now())
        )
        with open(self.release_info_file, 'w') as f:
            releases_dicts = {}
            for name, release in self.releases.items():
                releases_dicts[name] = release._asdict()

            json.dump(releases_dicts, f, indent=4)
