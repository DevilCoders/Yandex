package aws

import (
	"net/http"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/credentials"
	"github.com/aws/aws-sdk-go/aws/credentials/stscreds"
	"github.com/aws/aws-sdk-go/aws/session"
)

func NewSession(
	creds *credentials.Credentials,
	region string,
	client *http.Client,
) *session.Session {
	return session.Must(session.NewSession(
		aws.NewConfig().
			WithCredentials(creds).
			WithRegion(region).
			WithLogLevel(aws.LogDebugWithHTTPBody).
			WithHTTPClient(client),
	))
}

func NewSessionAssumeRole(
	ses *session.Session,
	roleArn string,
	region string,
	client *http.Client,
) *session.Session {
	creds := stscreds.NewCredentials(ses, roleArn)
	return NewSession(creds, region, client)
}
