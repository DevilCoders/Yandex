package tags

import (
	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/ec2"
)

func newTag(key, value string) *ec2.Tag {
	return &ec2.Tag{
		Key:   aws.String(key),
		Value: aws.String(value),
	}
}
