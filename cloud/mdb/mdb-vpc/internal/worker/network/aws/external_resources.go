package aws

import (
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models"
	"a.yandex-team.ru/cloud/mdb/mdb-vpc/internal/vpcdb/models/aws"
)

func CreateStateToExternalResources(state *aws.CreateNetworkOperationState) models.ExternalResources {
	return &aws.NetworkExternalResources{
		VpcID:           state.Vpc.ID,
		Subnets:         state.Subnets,
		IgwID:           state.IgwID,
		SecurityGroupID: state.SecurityGroupID,
		TgwAttachmentID: state.TgwAttachmentID,
		HostedZoneID:    &state.HostedZoneID,
	}
}

func ExternalResourcesToDeleteState(er models.ExternalResources, state *aws.DeleteNetworkOperationState) {
	e := er.(*aws.NetworkExternalResources)
	state.VpcID = e.VpcID
	state.Subnets = e.Subnets
	state.IgwID = e.IgwID
	state.SecurityGroupID = e.SecurityGroupID
	state.TgwAttachmentID = e.TgwAttachmentID
	state.HostedZoneID = e.HostedZoneID
	state.AccountID = e.AccountID
	state.IamRoleArn = e.IamRoleArn
}

func ImportVPCStateToExternalResources(state *aws.ImportVPCOperationState) models.ExternalResources {
	return &aws.NetworkExternalResources{
		VpcID:           state.Vpc.ID,
		Subnets:         state.Subnets,
		IgwID:           state.IgwID,
		SecurityGroupID: state.SecurityGroupID,
		TgwAttachmentID: state.TgwAttachmentID,
		HostedZoneID:    &state.HostedZoneID,
		AccountID:       optional.NewString(state.AccountID),
		IamRoleArn:      optional.NewString(state.IamRoleArn),
	}
}
