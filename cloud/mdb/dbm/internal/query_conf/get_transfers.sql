SELECT
    id,
    src_dom0,
    dest_dom0,
    container,
    placeholder,
    started
FROM
    mdb.transfers
ORDER BY
    started
