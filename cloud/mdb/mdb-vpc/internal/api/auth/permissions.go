package auth

type Permission string

var (
	NetworkCreatePermission Permission = "dcnetwork.networks.create"
	NetworkDeletePermission Permission = "dcnetwork.networks.delete"
	NetworkGetPermission    Permission = "dcnetwork.networks.get"

	NetworkConnectionCreatePermission Permission = "dcnetwork.networkConnections.create"
	NetworkConnectionDeletePermission Permission = "dcnetwork.networkConnections.delete"
	NetworkConnectionGetPermission    Permission = "dcnetwork.networkConnections.get"
)
