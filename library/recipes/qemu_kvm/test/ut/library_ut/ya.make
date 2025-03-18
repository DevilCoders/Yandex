OWNER(
    dmitko
    g:yatool
)

UNITTEST_FOR(devtools/dummy_arcadia/test/library_ut/simple_ut)

SRCS(
    main.cpp
)
INCLUDE(${ARCADIA_ROOT}/library/recipes/qemu_kvm/recipe.inc)
END()
