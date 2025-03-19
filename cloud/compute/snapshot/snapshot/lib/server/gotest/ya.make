GO_TEST_FOR(cloud/compute/snapshot/snapshot/lib/server)

OWNER(g:cloud-nbs)

FROM_SANDBOX(
    1358547184
    OUT_NOAUTO
    cirros-0.3.5-x86_64-disk.img
)

DATA(sbr://1358547184)

END()
