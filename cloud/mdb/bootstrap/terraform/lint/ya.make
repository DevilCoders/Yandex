EXECTEST()

OWNER(g:mdb)

# Can't simply run `terraform fmt -check`,
# cause terraform prints diff to stdout, ya make -t doesn't show stdout
RUN(
    NAME terraform-fmt
    sh -c 'terraform fmt -check -diff -recursive 1>&2'
    ENV PATH=/bin:/usr/bin:${ARCADIA_BUILD_ROOT}/cloud/mdb/bootstrap/terraform/lint/terraform:/usr/local/bin
    CWD ${ARCADIA_ROOT}/cloud/mdb/bootstrap/terraform/
)

DATA(arcadia/cloud/mdb/bootstrap/terraform)
DEPENDS(cloud/mdb/bootstrap/terraform/lint/terraform)

END()

RECURSE(terraform)
