INSERT INTO mdb.transfers (
    id,
    src_dom0,
    dest_dom0,
    container,
    placeholder
) VALUES (
    %(id)s,
    %(src_dom0)s,
    %(dest_dom0)s,
    %(container)s,
    %(placeholder)s
)
