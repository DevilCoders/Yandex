GO_LIBRARY()

OWNER(g:mdb)

PEERDIR("vendor/github.com/aws/aws-sdk-go/service/route53")

SRCS(route53.go)

END()
