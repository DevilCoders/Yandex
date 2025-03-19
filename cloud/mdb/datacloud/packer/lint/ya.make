EXECTEST()

OWNER(g:mdb)

# Can't simply run `packer fmt -check`,
# cause packer prints diff to stdout, ya make -t doesn't show stdout
RUN(
    NAME packer-fmt
    sh -c 'packer fmt -check -diff -recursive . 1>&2'
    ENV PATH=/bin:/usr/bin:${ARCADIA_BUILD_ROOT}/cloud/mdb/datacloud/packer/lint/packer:/usr/local/bin
    CWD ${ARCADIA_ROOT}/cloud/mdb/datacloud/packer/
)

DATA(arcadia/cloud/mdb/datacloud/packer)
DEPENDS(cloud/mdb/datacloud/packer/lint/packer)

END()

RECURSE(packer)
