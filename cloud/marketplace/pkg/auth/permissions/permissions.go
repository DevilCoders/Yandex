package permissions

type Permission string

const (
	LicenseCheckPermission Permission = "marketplaceinternal.licenses.check"
)

func (p Permission) String() string {
	return string(p)
}
