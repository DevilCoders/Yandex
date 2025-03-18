OWNER(g:java-commitee)

RECURSE(
    annotations
    client
    ds
    hnsw
    hnsw/jni
    monlib
    svnversion
    tvmauth
)

IF (NOT SANITIZER_TYPE)
     RECURSE(
         awssdk-extensions
     )
ENDIF()
