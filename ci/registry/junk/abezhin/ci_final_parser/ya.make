PY3_PROGRAM()

OWNER(g:tasklet)

PY_MAIN(tasklet.cli.main)

PEERDIR(
    ci/registry/junk/abezhin/ci_final_parser/proto
    ci/registry/junk/abezhin/ci_final_parser/lib
    tasklet/cli
)

END()