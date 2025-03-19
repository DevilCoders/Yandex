OWNER(
    ilnurkh
    insight
    g:neural-search
)

EXECTEST()


RUN(
    optimized_multiply_add_ut
)

RUN(
    bench --format csv
)

DEPENDS(
    kernel/dssm_applier/nn_applier/lib/optimized_multiply_add/tests/ut_impl
    kernel/dssm_applier/nn_applier/lib/optimized_multiply_add/bench
)

SIZE(LARGE)

TAG(
    ya:fat
    ya:force_sandbox
    sb:intel_e5_2660v4
)

END()
