package awstvm_test

import (
	"fmt"
	"log"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/s3"

	"a.yandex-team.ru/library/go/yandex/awstvm"
	"a.yandex-team.ru/library/go/yandex/tvm/tvmtool"
)

func ExampleNewS3Credentials() {
	tvmTool, err := tvmtool.NewAnyClient()
	if err != nil {
		log.Fatalf("failed to create tvm client: %v", err)
		return
	}

	awsCred, err := awstvm.NewS3Credentials(tvmTool, awstvm.WithAccessKeyID("TVM_V2_{my_tvm_client_id}"))
	if err != nil {
		log.Fatalf("failed to create AWS credentials: %v", err)
		return
	}

	s3Session, err := session.NewSession()
	if err != nil {
		log.Fatalf("failed to create S3 session: %v", err)
	}

	s3Client := s3.New(s3Session, aws.NewConfig().
		WithRegion("yandex").
		WithEndpoint("s3.mds.yandex.net").
		WithCredentials(awsCred))

	result, err := s3Client.ListBuckets(nil)
	if err != nil {
		log.Fatalf("unable to list buckets: %v", err)
	}

	fmt.Println(result)
}
