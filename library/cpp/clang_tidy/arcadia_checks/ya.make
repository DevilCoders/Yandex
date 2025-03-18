LIBRARY()

OWNER(g:yatool g:cpp-contrib)

PEERDIR(
    contrib/libs/clang14/lib/AST
    contrib/libs/clang14/lib/ASTMatchers
)

NO_COMPILER_WARNINGS()

NO_UTIL()

SRCS(
    taxi_coroutine_unsafe_check.cpp
    taxi_dangling_config_ref_check.cpp
    taxi_mt_unsafe_check.cpp
    GLOBAL tidy_module.cpp
    usage_restriction_checks.cpp
)

END()
