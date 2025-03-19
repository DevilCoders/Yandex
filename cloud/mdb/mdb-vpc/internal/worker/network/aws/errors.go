package aws

import "github.com/aws/aws-sdk-go/aws/awserr"

const (
	RouteAlreadyExists = "RouteAlreadyExists"
	RouteNotFound      = "InvalidRoute.NotFound"

	IgwNotFound        = "InvalidInternetGatewayID.NotFound"
	IgwAlreadyDetached = "Gateway.NotAttached"

	SubnetNotFound      = "InvalidSubnetID.NotFound"
	SubnetAlreadyExists = "InvalidSubnet.Conflict"

	VpcNotFound = "InvalidVpcID.NotFound"

	SecurityGroupRuleAlreadyExists = "InvalidPermission.Duplicate"
	SecurityGroupNotFound          = "InvalidGroup.NotFound"

	DNSZoneAlreadyAttached = "ConflictingDomainExists"
	DNSZoneAlreadyDetached = "VPCAssociationNotFound"

	TgwRouteTableAlreadyAssociated         = "Resource.AlreadyAssociated"
	TgwRouteTablePropagationAlreadyEnabled = "TransitGatewayRouteTablePropagation.Duplicate"
	TgwAttachmentNotFound                  = "InvalidTransitGatewayAttachmentId.NotFound"

	PeeringAlreadyExists = "VpcPeeringConnectionAlreadyExists"
	PeeringNotFound      = "InvalidVpcPeeringConnectionID.NotFound"

	ShareAlreadyAccepted = "TODO"
)

func getAwsErrorCode(err error) (string, bool) {
	awsErr, ok := err.(awserr.Error)
	if !ok {
		return "", false
	}
	return awsErr.Code(), true
}
