PY3_PROGRAM(sfx)

OWNER(orivej shadchin)

PEERDIR(
    library/python/sfx
)

PY_MAIN(library.python.sfx.main)

# For self test.
RESOURCE_FILES(ya.make)

END()
