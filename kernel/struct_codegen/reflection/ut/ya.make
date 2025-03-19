OWNER(
    mvel
    g:base
)

UNITTEST()

PEERDIR(
    ADDINCL kernel/struct_codegen/reflection
)

SRCDIR(kernel/struct_codegen/reflection)

SRCS(
    reflection_ut.cpp
)

END()
