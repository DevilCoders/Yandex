package aws

import (
	"net/http"

	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/ec2"
	"github.com/aws/aws-sdk-go/service/ec2/ec2iface"
	"github.com/aws/aws-sdk-go/service/ram"
	"github.com/aws/aws-sdk-go/service/ram/ramiface"
	"github.com/aws/aws-sdk-go/service/route53"
	"github.com/aws/aws-sdk-go/service/route53/route53iface"
)

type Providers struct {
	Controlplane *Provider
	Dataplane    *Provider

	cpSes  *session.Session
	region string
	client *http.Client
}

type Provider struct {
	Ec2     ec2iface.EC2API
	Route53 route53iface.Route53API
	RAM     ramiface.RAMAPI
}

func (p *Providers) AssumedDataplane(iamRole string) *Provider {
	newDPSes := NewSessionAssumeRole(p.cpSes, iamRole, p.region, p.client)
	return NewProvider(newDPSes)
}

func NewProviders(cpSes *session.Session, dpSes *session.Session, region string, client *http.Client) Providers {
	return Providers{
		Controlplane: NewProvider(cpSes),
		Dataplane:    NewProvider(dpSes),
		cpSes:        cpSes,
		region:       region,
		client:       client,
	}
}

func NewProvider(ses *session.Session) *Provider {
	return &Provider{
		Ec2:     ec2.New(ses),
		Route53: route53.New(ses),
		RAM:     ram.New(ses),
	}
}
