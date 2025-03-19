INSERT INTO mdb.clusters (
    name,
    project
) VALUES (
    %(cluster)s,
    %(project)s
) ON CONFLICT (name) DO NOTHING
