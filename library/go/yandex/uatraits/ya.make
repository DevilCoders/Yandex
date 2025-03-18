GO_LIBRARY()

OWNER(
    g:go-library
    yanykin
)

SRCS(
    branch.go
    common.go
    define.go
    extra_rules.go
    match.go
    pattern.go
    profile.go
    traits.go
    uatraits.go
    version.go
    xml_parser.go
    xml_structs.go
)

GO_TEST_SRCS(
    extra_rules_test.go
    profile_test.go
    traits_test.go
    uatraits_test.go
    version_test.go
    xml_parser_test.go
)

GO_XTEST_SRCS(example_test.go)

RESOURCE(
    metrika/uatraits/data/browser.xml browser.xml
    metrika/uatraits/data/extra.xml extra.xml
    metrika/uatraits/data/profiles.xml profiles.xml
)

END()

RECURSE(
    gotest
    test_generator
)
