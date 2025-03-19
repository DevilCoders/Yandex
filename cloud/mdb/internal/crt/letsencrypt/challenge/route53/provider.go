package route53

import (
	"context"
	"fmt"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/credentials"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/route53"
	"golang.org/x/crypto/acme"

	"a.yandex-team.ru/cloud/mdb/internal/crt/letsencrypt/challenge"
	"a.yandex-team.ru/cloud/mdb/internal/secret"
)

type Route53DNSChallengeProvider struct {
	*route53.Route53
	config Config
}

type Config struct {
	AWSAccessKeyID string        `json:"aws_access_key_id" yaml:"aws_access_key_id"`
	AWSSecretKey   secret.String `json:"aws_secret_key" yaml:"aws_secret_key"`
	HostedZoneID   string        `json:"hosted_zone_id" yaml:"hosted_zone_id"`
}

func New(config Config) challenge.ChallengeProvider {
	route53Client := route53.New(session.Must(session.NewSession()), &aws.Config{Credentials: credentials.NewStaticCredentials(
		config.AWSAccessKeyID,
		config.AWSSecretKey.Unmask(),
		"",
	)})

	return &Route53DNSChallengeProvider{Route53: route53Client, config: config}
}

var _ challenge.ChallengeProvider = &Route53DNSChallengeProvider{}

func (r Route53DNSChallengeProvider) ChallengeType() string {
	return "dns-01"
}

func (r Route53DNSChallengeProvider) Prepare(ctx context.Context, acme *acme.Client, auth acme.Authorization, challenge acme.Challenge) error {
	txtLabel := "_acme-challenge." + auth.Identifier.Value + "."
	txtValue, _ := acme.DNS01ChallengeRecord(challenge.Token)
	txtValue = fmt.Sprintf("\"%s\"", txtValue)
	return r.modifyTXTRecord(ctx, "UPSERT", txtLabel, txtValue, true)
}

func (r Route53DNSChallengeProvider) Cleanup(ctx context.Context, acme *acme.Client, auth acme.Authorization, challenge acme.Challenge) error {
	txtLabel := "_acme-challenge." + auth.Identifier.Value + "."
	txtValue, _ := acme.DNS01ChallengeRecord(challenge.Token)
	txtValue = fmt.Sprintf("\"%s\"", txtValue)
	return r.modifyTXTRecord(ctx, "DELETE", txtLabel, txtValue, false)
}

func (r *Route53DNSChallengeProvider) modifyTXTRecord(ctx context.Context, action, txtLabel, txtValue string, wait bool) error {
	recordType := "TXT"
	ttl := int64(300)
	changeResponse, err := r.ChangeResourceRecordSetsWithContext(ctx, &route53.ChangeResourceRecordSetsInput{
		HostedZoneId: &r.config.HostedZoneID,
		ChangeBatch: &route53.ChangeBatch{
			Changes: []*route53.Change{{
				Action: &action,
				ResourceRecordSet: &route53.ResourceRecordSet{
					Name: &txtLabel,
					Type: &recordType,
					TTL:  &ttl,
					ResourceRecords: []*route53.ResourceRecord{{
						Value: &txtValue,
					}},
				},
			}},
		},
	})
	if err != nil {
		return err
	}

	if wait {
		if err := r.WaitUntilResourceRecordSetsChangedWithContext(ctx, &route53.GetChangeInput{
			Id: changeResponse.ChangeInfo.Id,
		}); err != nil {
			return err
		}
	}

	return nil
}
