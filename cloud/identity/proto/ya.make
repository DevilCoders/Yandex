OWNER(g:cloud)

PROTO_LIBRARY()

GRPC()

SRCS(
    iam/v1/user_account.proto
    iam/v1/user_account_service.proto
    resourcemanager/v1/folder.proto
    resourcemanager/v1/transitional/folder_service.proto
    yrm/v1/folder.proto
    yrm/v1/transitional/folder_service.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
