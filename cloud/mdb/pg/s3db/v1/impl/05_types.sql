CREATE TYPE v1_impl.object_listing_item AS (
    object s3.objects,
    start_after text,
    level integer
);

CREATE TYPE v1_impl.object_part_listing_item AS (
    object_part s3.object_parts,
    start_after_key text,
    start_after_created timestamptz,
    level integer
);

CREATE TYPE v1_impl.list_name_item AS (
    bid uuid,
    name text,
    start_marker text,
    is_prefix boolean,
    level integer
);
