OWNER(g:cloud-asr)

PY3_PROGRAM(process_sbs_results)

PEERDIR(
    ml/nirvana/nope
    contrib/python/scipy
)

PY_SRCS(
    operation.py
)

PY_MAIN(nope.entry_point)

END()
