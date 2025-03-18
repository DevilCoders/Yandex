INCLUDE(${ARCADIA_ROOT}/contrib/java/com/squareup/okhttp3/okhttp-bom/4.9.2/ya.dependency_management.inc)

DEPENDENCY_MANAGEMENT(
    contrib/java/com/fasterxml/jackson/module/jackson-module-kotlin/2.13.0
    contrib/java/com/nhaarman/mockitokotlin2/mockito-kotlin/2.2.0
    contrib/java/com/squareup/retrofit2/converter-jackson/2.9.0
    contrib/java/com/squareup/retrofit2/retrofit/2.9.0
    contrib/java/com/squareup/retrofit2/retrofit-mock/2.9.0
    contrib/java/org/junit/vintage/junit-vintage-engine/5.5.2 # https://github.com/square/okhttp/issues/6770
)
