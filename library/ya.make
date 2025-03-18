OWNER(g:cpp-contrib)

RECURSE(
    c
    cpp
    python
    recipes
)

IF (NOT SANITIZER_TYPE)
    RECURSE(
        go
        java
    )
ENDIF()

NEED_CHECK()
