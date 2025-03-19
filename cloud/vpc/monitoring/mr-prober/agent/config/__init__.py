import abc
import logging
import pathlib
import shutil
import tempfile
from collections import defaultdict
from dataclasses import dataclass
from typing import Dict

import settings
from agent.config.models import AgentConfig, Prober, ProberFile, ProberWithConfig
from agent.config.s3.downloader import AgentConfigS3Downloader

__all__ = [
    "AgentConfigError", "AbstractAgentConfigLoader", "S3AgentConfigLoader", "StaticAgentConfigLoader"
]


@dataclass
class ProberFileOnDisk:
    relative_file_path: str
    md5_hexdigest: str
    location: pathlib.Path


class AgentConfigError(Exception):
    pass


class AbstractAgentConfigLoader(abc.ABC):
    @abc.abstractmethod
    def load(self) -> AgentConfig:
        pass


class S3AgentConfigLoader(AbstractAgentConfigLoader):
    """
    Loads config from Object Storage aka S3
    """

    def __init__(self):
        super().__init__()

        self.s3_downloader = AgentConfigS3Downloader(
            settings.S3_ENDPOINT,
            settings.S3_ACCESS_KEY_ID,
            settings.S3_SECRET_ACCESS_KEY,
            settings.AGENT_CONFIGURATIONS_S3_BUCKET,
            settings.S3_PREFIX,
        )

        # File location caches: we use it to avoid re-downloading files which are already downloaded
        self._prober_files_locations: Dict[int, pathlib.Path] = {}
        self._prober_files_on_disk: Dict[int, Dict[str, ProberFileOnDisk]] = defaultdict(dict)

    def load(self, update_files_on_disk: bool = True) -> AgentConfig:
        config = self.s3_downloader.get_config(settings.CLUSTER_ID, settings.HOSTNAME)

        if update_files_on_disk:
            for prober_with_config in config.probers:
                self.update_prober_files_on_disk_if_needed(prober_with_config.prober)

        return config

    def update_prober_files_on_disk_if_needed(self, prober: Prober):
        if self._are_prober_files_on_disk_in_actual_state(prober):
            # If all files are in actual state, get current files location (or create new one if it's first run)
            if prober.id in self._prober_files_locations:
                prober.files_location = self._prober_files_locations[prober.id]
            else:
                new_files_location = self._generate_prober_files_tmp_path(prober)
                prober.files_location = self._prober_files_locations[prober.id] = new_files_location
            return

        prober_files_on_disk = self._prober_files_on_disk[prober.id]
        new_files_location = self._generate_prober_files_tmp_path(prober)
        for file in prober.files:
            if file.relative_file_path in prober_files_on_disk:
                prober_file_on_disk = prober_files_on_disk[file.relative_file_path]
                if prober_file_on_disk.md5_hexdigest == file.md5_hexdigest:
                    # File not changed, just copy from previous location
                    try:
                        shutil.copy(prober_file_on_disk.location, new_files_location / file.relative_file_path)
                        continue
                    except Exception as e:
                        logging.warning(
                            "Can't copy prober file from %s to %s: %s",
                            prober_file_on_disk.location, new_files_location / file.relative_file_path, e,
                            exc_info=e
                        )

            # File changed or copying failed, re-download it from S3
            try:
                self.s3_downloader.download_and_store_prober_file(prober, file, new_files_location)
            except Exception as e:
                logging.error("Can't download prober file from S3: %s", e, exc_info=e)
                return

            path = new_files_location / file.relative_file_path
            mode = 0o755 if file.is_executable else 0o644
            path.chmod(mode)
            logging.debug("Set permissions %s for: %s", oct(path.stat().st_mode), path.name)

        self._prober_files_on_disk[prober.id] = {
            file.relative_file_path: ProberFileOnDisk(
                file.relative_file_path,
                file.md5_hexdigest,
                new_files_location / file.relative_file_path
            ) for file in prober.files
        }
        self._prober_files_locations[prober.id] = new_files_location
        prober.files_location = new_files_location

    @staticmethod
    def _generate_prober_files_tmp_path(prober: Prober) -> pathlib.Path:
        directory = tempfile.mkdtemp(dir=settings.PROBER_FILES_PATH, prefix=prober.slug + "_")
        return pathlib.Path(directory)

    def _are_prober_files_on_disk_in_actual_state(self, prober: Prober):
        prober_files_on_disk = self._prober_files_on_disk[prober.id]

        if len(prober_files_on_disk) != len(prober.files):
            return False

        for file in prober.files:
            if file.relative_file_path not in prober_files_on_disk:
                return False
            if prober_files_on_disk[file.relative_file_path].md5_hexdigest != file.md5_hexdigest:
                return False

        return True


class StaticAgentConfigLoader(AbstractAgentConfigLoader):
    """
    Simple loader which "loads" passed config. Used in tests.
    """

    def __init__(self, config: AgentConfig):
        super().__init__()
        self.config = config

    def load(self) -> AgentConfig:
        return self.config
