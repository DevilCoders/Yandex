package aws

import (
	"github.com/aws/aws-sdk-go/service/ec2"

	"a.yandex-team.ru/cloud/mdb/internal/cloud/aws/tags"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
)

func (s *Service) NetworkTags(resource string, projectID string, networkName string, networkID string) []*ec2.TagSpecification {
	ec2Tags := tags.DefaultDataPlaneTags(s.cfg.Name, s.cfg.VersionID, projectID, s.cfg.DiscountTag)
	ec2Tags = append(
		ec2Tags,
		tags.Namef("%s: %s", projectID, networkName),
		tags.NetworkID(networkID),
		tags.NetworkName(networkName),
	)

	var res ec2.TagSpecification
	res.SetResourceType(resource)
	res.SetTags(ec2Tags)
	return []*ec2.TagSpecification{&res}
}

func (s *Service) NetworkConnectionTags(resource string, op models.Operation, networkName string, networkID string, nc models.NetworkConnection) []*ec2.TagSpecification {
	ec2Tags := tags.DefaultDataPlaneTags(s.cfg.Name, s.cfg.VersionID, op.ProjectID, s.cfg.DiscountTag)
	ec2Tags = append(
		ec2Tags,
		tags.Namef("%s: %s", op.ProjectID, nc.ID),
		tags.NetworkID(networkID),
		tags.NetworkName(networkName),
		tags.NetworkConnectionID(nc.ID),
	)

	var res ec2.TagSpecification
	res.SetResourceType(resource)
	res.SetTags(ec2Tags)
	return []*ec2.TagSpecification{&res}
}
