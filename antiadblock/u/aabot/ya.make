OWNER(g:antiadblock)

GO_PROGRAM()

SRCS (
    main.go
    utils/tvmauth.go
    utils/utils.go
    utils/api.go
    utils/decrypt_urls.go
    chat/chat.go
    argus/argus.go
)

PEERDIR (
    library/cpp/tvmauth/client
    vendor/gopkg.in/tucnak/telebot.v2
)

END()
