package aws

import (
	"net/http"

	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/ec2"
	"github.com/aws/aws-sdk-go/service/ec2/ec2iface"
	"github.com/aws/aws-sdk-go/service/iam"
	"github.com/aws/aws-sdk-go/service/iam/iamiface"

	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/api/validation"
	internalaws "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/providers/aws"
)

type Validator struct {
	cpSes                   *session.Session
	client                  *http.Client
	defaultDataplaneIAMRole string

	ec2CliFactory func(iamRole string, region string, client *http.Client) ec2iface.EC2API
	iamCliFactory func(iamRole string, region string, client *http.Client) iamiface.IAMAPI
}

func NewValidator(cpSes *session.Session, client *http.Client, role string, opts ...Option) validation.Validator {
	v := &Validator{cpSes: cpSes, client: client, defaultDataplaneIAMRole: role}
	v.ec2CliFactory = v.ec2CliDefaultFactory
	v.iamCliFactory = v.iamCliDefaultFactory

	for _, opt := range opts {
		opt(v)
	}
	return v
}

func (v *Validator) ec2CliDefaultFactory(iamRole string, region string, client *http.Client) ec2iface.EC2API {
	dpSes := internalaws.NewSessionAssumeRole(
		v.cpSes,
		iamRole,
		region,
		client,
	)
	return ec2.New(dpSes)
}

func (v *Validator) iamCliDefaultFactory(iamRole string, region string, client *http.Client) iamiface.IAMAPI {
	dpSes := internalaws.NewSessionAssumeRole(
		v.cpSes,
		iamRole,
		region,
		client,
	)
	return iam.New(dpSes)
}

type Option func(v *Validator)

func WithEc2CliFactory(f func(iamRole string, region string, client *http.Client) ec2iface.EC2API) Option {
	return func(v *Validator) {
		v.ec2CliFactory = f
	}
}

func WithIamCliFactory(f func(iamRole string, region string, client *http.Client) iamiface.IAMAPI) Option {
	return func(v *Validator) {
		v.iamCliFactory = f
	}
}
