package route53

import (
	"context"
	"os"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/credentials"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/route53"

	"a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/dnsq"
	"a.yandex-team.ru/library/go/core/log"
)

var _ dnsq.Client = &Client{}

type Client struct {
	logger       log.Logger
	r53Client    *route53.Route53
	hostedZoneID string
}

func New(logger log.Logger, hostedZoneID string) *Client {
	awsCredentials := credentials.NewStaticCredentials(
		os.Getenv("AWS_ROUTE53_ACCESS_KEY"),
		os.Getenv("AWS_ROUTE53_SECRET_KEY"),
		"",
	)
	awsConfig := &aws.Config{
		Credentials: awsCredentials,
	}
	r53Client := route53.New(session.Must(session.NewSession()), awsConfig)
	return &Client{
		logger:       logger,
		r53Client:    r53Client,
		hostedZoneID: hostedZoneID,
	}
}

func (c *Client) LookupCNAME(ctx context.Context, fqdn, groupid string) (string, bool) {
	reqInput := route53.ListResourceRecordSetsInput{
		HostedZoneId:    aws.String(c.hostedZoneID),
		MaxItems:        aws.String("1"),
		StartRecordName: aws.String(fqdn),
		StartRecordType: aws.String("CNAME"),
	}
	c.logger.Debugf("Request for list records, %v", reqInput)
	res, err := c.r53Client.ListResourceRecordSets(&reqInput)
	if err != nil {
		c.logger.Errorf("failed to get list records: %s", err)
		return "", false
	}
	recordSets := res.ResourceRecordSets
	if len(recordSets) < 1 {
		return "", true
	}
	if *recordSets[0].Name != fqdn {
		// No target fqdn in results, and recordSets[0] contains just next record
		// (because ListResourceRecordSetsInput returns values starting from specified, not only specified)
		return "", true
	}
	cnames := recordSets[0].ResourceRecords
	if len(cnames) == 0 {
		return "", true
	}
	if len(cnames) > 1 {
		c.logger.Warnf("got few cnames for FQDN: %s, records count: %d", fqdn, len(cnames))
	}
	cname := *cnames[0].Value
	return cname, true
}
