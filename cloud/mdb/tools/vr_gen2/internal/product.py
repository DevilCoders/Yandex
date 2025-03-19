from typing import List, Generator, NamedTuple

from dataclasses import fields
from humanfriendly import parse_size

from .data_sources import FlavorSourceBase, DiskTypeIdSourceBase, GeoIdSourceBase
from .disk_size import from_disk_size
from .models.base import ValidResourceDef
from .models.resource import Resource
from .schema import ResourceDefSchema


def flavor_axis(resource_def: ValidResourceDef, flavor_source: FlavorSourceBase) -> List[str]:
    flavors = set()
    for flavor_type in resource_def.flavor_types:
        if flavor_type.name:  # TODO: support regexp here when needed
            flavors.update(set(flavor_source.filter_ids_by_name(flavor_type.name)))
        else:
            flavors.update(set(flavor_source.filter_ids_by_type(flavor_type.type)))
    return list(flavors)


class Vector(NamedTuple):
    disk_type_id: int
    cluster_type: str
    role: str


def produce_resources_from_def(
    resource_def: ValidResourceDef,
    flavor_source: FlavorSourceBase,
    geo_source: GeoIdSourceBase,
    vector: Vector,
) -> Generator[Resource, None, None]:
    default_disk_size = (
        parse_size(resource_def.default_disk_size, binary=True) if resource_def.default_disk_size else None
    )
    for host_count in resource_def.host_count:
        for flavor in flavor_axis(resource_def, flavor_source):
            for d_size in resource_def.disk_sizes:
                disk_size_range, disk_sizes = from_disk_size(d_size)
                for geo_id in geo_source.get_without_names(resource_def.excluded_geo):
                    for feature_flag in resource_def.feature_flags or [None]:
                        yield Resource(
                            cluster_type=vector.cluster_type,
                            role=vector.role,
                            max_hosts=host_count.max,
                            min_hosts=host_count.min,
                            flavor=flavor,
                            disk_type_id=vector.disk_type_id,
                            geo_id=geo_id,
                            id=0,
                            default_disk_size=default_disk_size,
                            disk_size_range=disk_size_range,
                            feature_flag=feature_flag,
                            disk_sizes=disk_sizes,
                        )


def id_seq() -> Generator[int, None, None]:
    cnt = 1
    while True:
        yield cnt
        cnt += 1


def generate_resources(
    resource_definition: ResourceDefSchema,
    geo_source: GeoIdSourceBase,
    flavor_source: FlavorSourceBase,
    disk_source: DiskTypeIdSourceBase,
) -> List[Resource]:
    ids = id_seq()
    result = []
    for cluster_type_field in fields(resource_definition.cluster_types):
        cluster_type = cluster_type_field.name
        role_definitions = getattr(resource_definition.cluster_types, cluster_type)
        for role_definition_field in fields(role_definitions):
            role = role_definition_field.name
            disk_definitions = getattr(role_definitions, role)
            if disk_definitions is None:
                continue
            for disk_def_field in fields(disk_definitions):
                disk_ext_id = disk_def_field.name
                resources = getattr(disk_definitions, disk_ext_id)
                for resource_def in resources:
                    vector = Vector(
                        disk_type_id=disk_source.get_by_ext_id(disk_ext_id),
                        cluster_type=cluster_type,
                        role=role,
                    )
                    for resource in produce_resources_from_def(resource_def, flavor_source, geo_source, vector):
                        resource.id = next(ids)
                        result.append(resource)
    return result
