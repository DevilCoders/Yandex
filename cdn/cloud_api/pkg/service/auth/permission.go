package auth

type Permission string

func (p Permission) String() string {
	return string(p)
}

const (
	createPermission = "cdn.resources.create"
	readPermission   = "cdn.resources.get"
	deletePermission = "cdn.resources.delete"
)

const (
	CreateResourcePermission Permission = createPermission
	GetResourcePermission    Permission = readPermission
	ListResourcesPermission  Permission = readPermission
	UpdateResourcePermission Permission = createPermission
	DeleteResourcePermission Permission = deletePermission

	CreateOriginPermission Permission = createPermission
	GetOriginPermission    Permission = readPermission
	UpdateOriginPermission Permission = createPermission
	DeleteOriginPermission Permission = createPermission
	ListOriginsPermission  Permission = readPermission
)
