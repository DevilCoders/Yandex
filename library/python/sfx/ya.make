PY23_LIBRARY()

OWNER(orivej shadchin)

PEERDIR(
    contrib/python/six
)

PY_SRCS(
    extract_program.py
    extract.py
    main.py
)

END()

RECURSE(
    bin
)

RECURSE_FOR_TESTS(
    test
    test_lib
)
