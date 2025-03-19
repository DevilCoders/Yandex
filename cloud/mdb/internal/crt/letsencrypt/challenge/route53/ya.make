GO_LIBRARY()

OWNER(g:mdb)

PEERDIR("vendor/github.com/aws/aws-sdk-go/service/route53")

SRCS(provider.go)

END()
