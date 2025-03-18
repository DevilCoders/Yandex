GO_TEST()

OWNER(
    prime
    g:go-library
)

DATA(arcadia/library/go/test/mutable_testdata/recipe_test)

DEPENDS(library/go/test/mutable_testdata)

USE_RECIPE(
    library/go/test/mutable_testdata/mutable_testdata
    --testdata-dir
    library/go/test/mutable_testdata/recipe_test
)

GO_XTEST_SRCS(recipe_test.go)

END()
