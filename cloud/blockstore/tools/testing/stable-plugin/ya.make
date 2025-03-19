UNION()

OWNER(g:cloud-nbs)

SET(RESOURCE_ID 3240550068)
SET(DEB_FILE yandex-cloud-blockstore-plugin_1351636689.releases.ydb.stable-22-2_amd64.deb)

FROM_SANDBOX(
    ${RESOURCE_ID}
    OUT_NOAUTO ${DEB_FILE}
)

PYTHON(
    ${CURDIR}/extract.py ${DEB_FILE}
    CWD ${BINDIR}
    IN_NOPARSE ${DEB_FILE}
    OUT_NOAUTO libblockstore-plugin.so
)

END()
