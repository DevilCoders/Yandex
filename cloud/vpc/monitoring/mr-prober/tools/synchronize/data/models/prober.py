import glob
import pathlib
from typing import Optional, List, Iterable, Dict

from pydantic import BaseModel

import api.models
import database.models
import tools.synchronize.iac.models as iac_models
from api.client import MrProberApiClient
from tools.synchronize.equalable import EqualableMixin
from tools.synchronize.models import FileCollection, VariableType
from tools.synchronize.iac.errors import IacDataError
from .cluster import Cluster
from .utils import are_file_contents_changed, save_files_to_disk, is_file_executable

__all__ = ["Prober", "ProberFile", "ProberConfig"]


class ProberFile(BaseModel, EqualableMixin):
    id: Optional[int] = None
    relative_file_path: str
    is_executable: bool
    content: bytes

    class Config:
        comparator_ignores_keys = {"id"}

    @classmethod
    def load_from_iac(cls, file_collection: FileCollection, base_path: pathlib.Path) -> Iterable["ProberFile"]:
        for relative_file_path, filename in file_collection.get_files(base_path):
            yield ProberFile(
                id=None,
                relative_file_path=relative_file_path.as_posix(),
                is_executable=is_file_executable(filename),
                content=filename.read_bytes(),
            )

    @classmethod
    def load_from_api_model(
        cls, client: MrProberApiClient, prober: api.models.Prober, file: api.models.ProberFile
    ) -> "ProberFile":
        return cls(
            id=file.id,
            relative_file_path=file.relative_file_path,
            is_executable=file.is_executable,
            content=client.probers.get_file_content(prober.id, file.id),
        )


class ProberConfig(BaseModel, EqualableMixin):
    id: Optional[int] = None
    is_prober_enabled: Optional[bool] = None
    interval_seconds: Optional[int] = None
    timeout_seconds: Optional[int] = None
    s3_logs_policy: Optional[database.models.UploadProberLogPolicy] = None
    default_routing_interface: Optional[str] = None
    dns_resolving_interface: Optional[str] = None
    cluster: Optional[Cluster] = None
    hosts_re: Optional[str] = None
    matrix_variables: Optional[Dict[str, List[VariableType]]] = None
    variables: Optional[Dict[str, VariableType]] = None

    class Config(BaseModel.Config):
        comparator_ignores_keys = {"id": ..., "cluster": set(Cluster.__fields__.keys()) - {"arcadia_path"}}

    @classmethod
    def load_from_iac(cls, config: iac_models.ProberConfig) -> Iterable["ProberConfig"]:
        result = cls(
            id=None,
            is_prober_enabled=config.is_prober_enabled,
            interval_seconds=config.interval_seconds,
            timeout_seconds=config.timeout_seconds,
            s3_logs_policy=config.s3_logs_policy,
            default_routing_interface=config.default_routing_interface,
            dns_resolving_interface=config.dns_resolving_interface,
            cluster=None,
            hosts_re=config.hosts_re,
            matrix_variables=config.matrix,
            variables=config.variables,
        )
        if config.clusters is None:
            yield result
            return

        for cluster_glob in config.clusters:
            for cluster_filename in glob.glob(cluster_glob, recursive=True):
                yield result.copy(
                    update={
                        "cluster": Cluster.load_from_iac(pathlib.Path(cluster_filename)),
                    }
                )

    @classmethod
    def load_from_api_model(
        cls, client: MrProberApiClient, config: api.models.ProberConfig,
        collection: Optional["ObjectsCollection"] = None
    ) -> "ProberConfig":
        return cls(
            id=config.id,
            is_prober_enabled=config.is_prober_enabled,
            interval_seconds=config.interval_seconds,
            timeout_seconds=config.timeout_seconds,
            s3_logs_policy=config.s3_logs_policy,
            default_routing_interface=config.default_routing_interface,
            dns_resolving_interface=config.dns_resolving_interface,
            cluster=Cluster.load_from_api(
                client, config.cluster_id, collection
            ) if config.cluster_id is not None else None,
            hosts_re=config.hosts_re,
            matrix_variables={v.name: v.values for v in config.matrix_variables} if config.matrix_variables else None,
            variables={v.name: v.value for v in config.variables} if config.variables else None,
        )

    def convert_to_iac_model(self) -> iac_models.ProberConfig:
        return iac_models.ProberConfig(
            is_prober_enabled=self.is_prober_enabled,
            interval_seconds=self.interval_seconds,
            timeout_seconds=self.timeout_seconds,
            s3_logs_policy=self.s3_logs_policy,
            default_routing_interface=self.default_routing_interface,
            dns_resolving_interface=self.dns_resolving_interface,
            hosts_re=self.hosts_re,
            cluster=self.cluster.arcadia_path if self.cluster is not None else None,
            matrix=self.matrix_variables,
            variables=self.variables,
        )


class Prober(BaseModel, EqualableMixin):
    id: Optional[int] = None
    arcadia_path: str
    name: str
    slug: str
    description: str
    runner: api.models.ProberRunner
    files: List[ProberFile]
    configs: List[ProberConfig]

    class Config:
        comparator_ignores_keys = {
            "id": ...,
            "configs": {"__all__": ProberConfig.Config.comparator_ignores_keys},
            "files": {"__all__": ProberFile.Config.comparator_ignores_keys}
        }

    @classmethod
    def load_from_iac(cls, arcadia_path: pathlib.Path) -> "Prober":
        prober = iac_models.Prober.load_from_file(arcadia_path)
        result = cls(
            id=None,
            arcadia_path=arcadia_path.as_posix(),
            name=prober.name,
            slug=prober.slug,
            description=prober.description,
            runner=prober.runner,
            files=[],
            configs=[]
        )
        for file_collection in prober.files:
            result.files.extend(ProberFile.load_from_iac(file_collection, arcadia_path.parent))
        for config in prober.configs:
            result.configs.extend(ProberConfig.load_from_iac(config))

        result.validate_files()

        return result

    def validate_files(self):
        file_by_relative_path = {}
        for file in self.files:
            if file.relative_file_path in file_by_relative_path:
                raise IacDataError(f"Prober {self.arcadia_path} has conflicted files on {file.relative_file_path}")
            file_by_relative_path[file.relative_file_path] = file

    @classmethod
    def load_from_api(
        cls, client: MrProberApiClient, prober_id: int, collection: Optional["ObjectsCollection"] = None
    ) -> "Prober":
        prober = client.probers.get(prober_id)
        return cls.load_from_api_model(client, prober, collection)

    @classmethod
    def load_from_api_model(
        cls, client: MrProberApiClient, prober: api.models.Prober, collection: Optional["ObjectsCollection"] = None
    ) -> "Prober":
        if collection is not None and prober.arcadia_path in collection.probers:
            return collection.probers[prober.arcadia_path]

        files = [ProberFile.load_from_api_model(client, prober, file) for file in prober.files]
        configs: List[ProberConfig] = []
        for config in sorted(prober.configs or [], key=lambda prober_config: prober_config.id):
            if config.manually_created:
                continue
            configs.append(ProberConfig.load_from_api_model(client, config, collection))

        return cls(
            id=prober.id,
            arcadia_path=prober.arcadia_path,
            name=prober.name,
            slug=prober.slug,
            description=prober.description,
            runner=prober.runner,
            files=files,
            configs=configs,
        )

    def delete_in_api(self, client: MrProberApiClient, collection: "ObjectsCollection"):
        assert self.id is not None

        client.probers.delete(self.id)

    def create_in_api(
        self, client: MrProberApiClient, collection: "ObjectsCollection", with_configs: bool = True
    ) -> "Prober":
        assert self.id is None

        created_prober = client.probers.create(
            api.models.CreateProberRequest(
                manually_created=False,
                arcadia_path=self.arcadia_path,
                name=self.name,
                slug=self.slug,
                description=self.description,
                runner=self.runner,
            )
        )

        for file in self.files:
            created_file = client.probers.add_file(
                created_prober.id, api.models.CreateProberFileRequest(
                    relative_file_path=file.relative_file_path,
                    is_executable=file.is_executable
                )
            )
            client.probers.upload_file_content(created_prober.id, created_file.id, file.content)

        if with_configs:
            self._create_configs_in_api(client, collection, self.configs, created_prober.id)

        return Prober.load_from_api(client, created_prober.id)

    def update_in_api(
        self, client: MrProberApiClient, collection: "ObjectsCollection", new_prober: "Prober"
    ) -> "Prober":
        assert self.id is not None

        if are_file_contents_changed(self.files, new_prober.files):
            if self._are_configs_changed(self.configs, new_prober.configs):
                # Create a copy with full set of configs. Current prober will be removed with their configs
                # some lines below.
                prober_copy = new_prober.create_in_api(client, collection)
            else:
                # Create a copy without configs and atomically move config from old one to the new one.
                prober_copy = new_prober.create_in_api(client, collection, with_configs=False)
                client.probers.move_configs(self.id, prober_copy.id)

            self.delete_in_api(client, collection)
            return prober_copy

        # File contents are the same, but others fields (i.e. is_executable) may change
        old_files_by_path = {file.relative_file_path: file for file in self.files}
        new_files_by_path = {file.relative_file_path: file for file in new_prober.files}
        for file_path, old_file in old_files_by_path.items():
            new_file = new_files_by_path[file_path]
            if not new_file.equals(old_file):
                client.probers.update_file(
                    self.id,
                    old_file.id,
                    api.models.UpdateProberFileRequest(
                        relative_file_path=new_file.relative_file_path,
                        is_executable=new_file.is_executable,
                        # We use force file changing here, it allows us to change is_executable field of ProberFile
                        # even if prober is attached to some config. We don't want to re-create
                        # all ProberFiles and Prober here, because changing of is_executable field is not so important.
                        # We haven't updated Prober.runner yet, so this changing should be safe.
                        force=True,
                    )
                )

        prober_from_api = client.probers.update(
            self.id,
            api.models.UpdateProberRequest(
                name=new_prober.name,
                manually_created=False,
                arcadia_path=new_prober.arcadia_path,
                slug=new_prober.slug,
                description=new_prober.description,
                runner=new_prober.runner,
            )
        )

        if self._are_configs_changed(self.configs, new_prober.configs):
            # Delete all configs and re-create them — there is no way to do it better now.
            # it's worth thinking about it later. I.e. change configs in-place, add new ones and delete only
            # configs deleted from IaC.
            for config in self.configs:
                client.probers.delete_config(self.id, config.id)
            self._create_configs_in_api(client, collection, new_prober.configs, self.id)

        return Prober.load_from_api_model(client, prober_from_api)

    @staticmethod
    def _create_configs_in_api(
        client: MrProberApiClient, collection: "ObjectCollection",
        configs: List[ProberConfig], prober_id: Optional[int]
    ):
        for config in configs:
            if config.cluster is not None:
                cluster = collection.clusters[config.cluster.arcadia_path]
            else:
                cluster = None

            client.probers.add_config(
                prober_id, api.models.CreateProberConfigRequest(
                    manually_created=False,

                    cluster_id=cluster.id if cluster is not None else None,
                    hosts_re=config.hosts_re,

                    is_prober_enabled=config.is_prober_enabled,
                    interval_seconds=config.interval_seconds,
                    timeout_seconds=config.timeout_seconds,
                    s3_logs_policy=config.s3_logs_policy,
                    default_routing_interface=config.default_routing_interface,
                    dns_resolving_interface=config.dns_resolving_interface,

                    matrix_variables=config.matrix_variables,
                    variables=config.variables,
                )
            )

    @staticmethod
    def _are_configs_changed(old_configs: List[ProberConfig], new_configs: List[ProberConfig]) -> bool:
        # TODO (andgein) There is a bug somewhere here: sometimes configs are equal, but tool recreates them
        old_configs_without_ids = {config.json(exclude={"id"}, sort_keys=True) for config in old_configs}
        new_configs_without_ids = {config.json(exclude={"id"}, sort_keys=True) for config in new_configs}

        return old_configs_without_ids != new_configs_without_ids

    def save_to_iac(self, path: pathlib.Path):
        path.parent.mkdir(parents=True, exist_ok=True)

        if self.files:
            save_files_to_disk(self.files, path.parent / "files")

        prober = iac_models.Prober(
            name=self.name,
            slug=self.slug,
            description=self.description,
            runner=self.runner,
            files=[FileCollection(directory="files/")] if self.files else [],
            configs=[config.convert_to_iac_model() for config in self.configs],
        )

        prober.save_to_file(path)
