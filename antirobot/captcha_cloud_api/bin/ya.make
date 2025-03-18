GO_PROGRAM(captcha_cloud_api)

OWNER(g:antirobot)

SRCS(
    auth.go
    main.go
    args.go
    gen_keys.go
    logging.go
    util.go
    resource_manager.go
    search_schema.go
    captcha_service.go
    stats_service.go
    operation_service.go
    quota_service.go
)

PEERDIR(
    antirobot/captcha_cloud_api/proto
)

END()
