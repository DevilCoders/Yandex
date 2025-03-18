PY3_PROGRAM()

OWNER(g:tasklet)

PY_MAIN(tasklet.cli.main)

PEERDIR(
    ci/registry/junk/bsheludko/update_bio_beta_tasklet/proto
    ci/registry/junk/bsheludko/update_bio_beta_tasklet/lib
    tasklet/cli
)

END()
