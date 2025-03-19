GO_PROGRAM()

SRCS(main.go)

RUN_PROGRAM(
    cloud/billing/go/bundler -s ${CURDIR}/../..
        ${BINDIR}/data test_bundle

    OUT
        data/units.yaml
        data/services.yaml
        data/schemas.yaml
        data/skus.yaml
)

RESOURCE(
    data/units.yaml units
    data/services.yaml services
    data/schemas.yaml schemas
    data/skus.yaml skus
)

END()
