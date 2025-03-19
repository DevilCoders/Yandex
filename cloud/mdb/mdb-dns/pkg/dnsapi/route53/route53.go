package route53

import (
	"context"
	"os"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/awserr"
	"github.com/aws/aws-sdk-go/aws/credentials"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/route53"

	cfg "a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/config"
	"a.yandex-team.ru/cloud/mdb/mdb-dns/pkg/dnsapi"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var _ dnsapi.Client = &Client{}

type Client struct {
	logger       log.Logger
	r53Client    *route53.Route53
	hostedZoneID string
	ttl          uint64
}

func New(l log.Logger, config cfg.R53Config, ttl uint64) *Client {
	awsCredentials := credentials.NewStaticCredentials(
		os.Getenv("AWS_ROUTE53_ACCESS_KEY"),
		os.Getenv("AWS_ROUTE53_SECRET_KEY"),
		"",
	)
	awsConfig := &aws.Config{Credentials: awsCredentials}
	r53Client := route53.New(session.Must(session.NewSession()), awsConfig)
	return &Client{
		logger:       l,
		r53Client:    r53Client,
		hostedZoneID: config.HostedZoneID,
		ttl:          ttl,
	}
}

func (c *Client) UpdateRecords(ctx context.Context, up *dnsapi.RequestUpdate) ([]*dnsapi.RequestUpdate, error) {
	changes := make([]*route53.Change, 0, len(up.Records))
	for cidfqdn, upInfo := range up.Records {
		rrset := route53.ResourceRecordSet{
			Name:            aws.String(cidfqdn),
			ResourceRecords: []*route53.ResourceRecord{{Value: aws.String(upInfo.CNAMENew)}},
			TTL:             aws.Int64(int64(c.ttl)),
			Type:            aws.String("CNAME"),
		}
		var action string
		if upInfo.CNAMENew == "" {
			action = "DELETE"
		} else {
			action = "UPSERT"
		}
		change := route53.Change{
			Action:            aws.String(action),
			ResourceRecordSet: &rrset,
		}
		changes = append(changes, &change)
	}
	batch := route53.ChangeBatch{Changes: changes}
	reqInput := route53.ChangeResourceRecordSetsInput{
		ChangeBatch:  &batch,
		HostedZoneId: &c.hostedZoneID,
	}
	c.logger.Debugf("send update DNS request to AWS Route 53, with input: %v", reqInput)
	_, err := c.r53Client.ChangeResourceRecordSets(&reqInput)
	if err != nil {
		var awsError awserr.Error
		if xerrors.As(err, &awsError) {
			switch awsError.Code() {
			case route53.ErrCodeNoSuchHostedZone, route53.ErrCodeInvalidInput:
				// Fatal error due to bad inputs
				wrerror := dnsapi.ErrInternal.Wrap(
					xerrors.Errorf("failed with code \"%q\": %w", awsError.Code(), err))
				return nil, wrerror
			default:
				return dnsapi.SplitOnParts(up), err
			}
		}
		return dnsapi.SplitOnParts(up), err
	}
	return nil, nil
}
