GO_PROGRAM(device_validator)

OWNER(g:antirobot)

SRCS(
    api_android.go
    api_ios.go
    api_utils.go
    args.go
    config.go
    ecdsa.go
    jwt.go
    logging.go
    main.go
    safetynet.go
    stats.go
    utils.go
)

END()

RECURSE(
    build
    proto
    tools
)
