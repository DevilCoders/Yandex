package aws_test

import (
	"testing"

	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/service/ec2"
	"github.com/stretchr/testify/require"

	awsproviders "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/providers/aws"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/worker/config"
	aws2 "a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/worker/network/aws"
	"a.yandex-team.ru/library/go/core/log/nop"
)

func Test_createTags(t *testing.T) {
	const (
		cpName      = "cp1"
		cpVersionID = "cpv1"
	)
	type input struct {
		resource    string
		projectID   string
		networkName string
		networkID   string
	}
	tcs := []struct {
		input  input
		output ec2.TagSpecification
	}{
		{
			input: input{
				resource:    "vpc",
				projectID:   "p1",
				networkName: "n1",
				networkID:   "nid1",
			},
			output: ec2.TagSpecification{
				ResourceType: aws.String("vpc"),
				Tags: []*ec2.Tag{
					{
						Key:   aws.String("Team"),
						Value: aws.String("MDB"),
					},
					{
						Key:   aws.String("Name"),
						Value: aws.String("p1: n1"),
					},
					{
						Key:   aws.String("Plane"),
						Value: aws.String("Data"),
					},
					{
						Key:   aws.String("ControlPlane"),
						Value: aws.String(cpName),
					},
					{
						Key:   aws.String("ControlPlaneVersionID"),
						Value: aws.String(cpVersionID),
					},
					{
						Key:   aws.String("ProjectID"),
						Value: aws.String("p1"),
					},
					{
						Key:   aws.String("NetworkName"),
						Value: aws.String("n1"),
					},
					{
						Key:   aws.String("NetworkID"),
						Value: aws.String("nid1"),
					},
					{
						Key:   aws.String("map-migrated"),
						Value: aws.String("qwerty"),
					},
					{
						Key:   aws.String("AtDoubleCloud"),
						Value: aws.String("true"),
					},
				},
			},
		},
		{
			input: input{
				resource:    "subnet",
				projectID:   "p1",
				networkName: "n1",
				networkID:   "nid1",
			},
			output: ec2.TagSpecification{
				ResourceType: aws.String("subnet"),
				Tags: []*ec2.Tag{
					{
						Key:   aws.String("Team"),
						Value: aws.String("MDB"),
					},
					{
						Key:   aws.String("Name"),
						Value: aws.String("p1: n1"),
					},
					{
						Key:   aws.String("Plane"),
						Value: aws.String("Data"),
					},
					{
						Key:   aws.String("ControlPlane"),
						Value: aws.String(cpName),
					},
					{
						Key:   aws.String("ControlPlaneVersionID"),
						Value: aws.String(cpVersionID),
					},
					{
						Key:   aws.String("ProjectID"),
						Value: aws.String("p1"),
					},
					{
						Key:   aws.String("NetworkName"),
						Value: aws.String("n1"),
					},
					{
						Key:   aws.String("NetworkID"),
						Value: aws.String("nid1"),
					},
					{
						Key:   aws.String("map-migrated"),
						Value: aws.String("qwerty"),
					},
					{
						Key:   aws.String("AtDoubleCloud"),
						Value: aws.String("true"),
					},
				},
			},
		},
	}
	s := aws2.NewCustomService(awsproviders.Providers{}, &nop.Logger{}, nil, config.AwsControlPlaneConfig{
		Name:        cpName,
		VersionID:   cpVersionID,
		DiscountTag: "qwerty",
	})
	for _, tc := range tcs {
		t.Run("", func(t *testing.T) {
			res := s.NetworkTags(tc.input.resource, tc.input.projectID, tc.input.networkName, tc.input.networkID)
			require.Len(t, res, 1)
			require.Equal(t, tc.output.ResourceType, res[0].ResourceType)
			require.ElementsMatch(t, tc.output.Tags, res[0].Tags)
		})
	}
}
