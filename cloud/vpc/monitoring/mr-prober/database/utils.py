from typing import TypeVar

from sqlalchemy.orm import class_mapper

from . import Base

__all__ = ["copy_object"]

T = TypeVar("T", bound=Base)


def copy_object(obj: T, omit_foreign_keys=True) -> T:
    """
    Helper for cloning objects in database.

    Given an SQLAlchemy object, creates a new object, and copies
    across all attributes, omitting primary keys, foreign keys (by default),
    and relationship attributes.

    Usage example:

    cluster = get_cluster(cluster_id)
    new_cluster = clone_object(cluster)
    db.add(new_cluster)
    db.commit()
    """
    cls = type(obj)
    mapper = class_mapper(cls)
    new_object = cls()
    primary_key_keys = {c.key for c in mapper.primary_key}
    relationship_keys = {c.key for c in mapper.relationships}
    prohibited_keys = primary_key_keys | relationship_keys
    if omit_foreign_keys:
        foreign_key_keys = {c.key for c in mapper.columns if c.foreign_keys}
        prohibited_keys |= foreign_key_keys

    for key in (p.key for p in mapper.iterate_properties if p.key not in prohibited_keys):
        try:
            value = getattr(obj, key)
            setattr(new_object, key, value)
        except AttributeError:
            pass

    return new_object
