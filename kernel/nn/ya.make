OWNER(g:factordev)

RECURSE(
    neural_ranker
)

IF (OS_LINUX)
    RECURSE(
        tf_apply
        tf_apply/ut
    )
ENDIF()
