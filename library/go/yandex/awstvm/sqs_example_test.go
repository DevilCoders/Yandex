package awstvm_test

import (
	"fmt"
	"log"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/sqs"

	"a.yandex-team.ru/library/go/yandex/awstvm"
	"a.yandex-team.ru/library/go/yandex/tvm/tvmtool"
)

func ExampleNewSqsCredentials() {
	tvmTool, err := tvmtool.NewAnyClient()
	if err != nil {
		log.Fatalf("failed to create tvm client: %v", err)
		return
	}

	awsCred, err := awstvm.NewSqsCredentials(tvmTool, "my_account")
	if err != nil {
		log.Fatalf("failed to create AWS credentials: %v", err)
		return
	}

	sess, err := session.NewSession(&aws.Config{
		Region:      aws.String("yandex"),
		Endpoint:    aws.String("https://sqs.yandex.net"),
		Credentials: awsCred,
	})

	if err != nil {
		log.Fatalf("failed to create SQS session: %v", err)
		return
	}

	sqsClient := sqs.New(sess)
	result, err := sqsClient.ListQueues(&sqs.ListQueuesInput{})
	if err != nil {
		log.Fatalf("failed to list queues: %v", err)
		return
	}

	fmt.Println(result)
}
