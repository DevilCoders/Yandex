package models

// action within Access service that permissions relate to
type ASAction int

const (
	ASGet ASAction = iota
	ASUpdate
	ASCreate
	ASDelete
	ASUse
)

// Access service permission
type Permission struct {
	Name      string
	Action    ASAction
	withForce *Permission
}

func NewPermissionWithForce(name string, action ASAction, perm *Permission) Permission {
	return Permission{
		Name:      name,
		Action:    action,
		withForce: perm,
	}
}

var (
	PermMDBQuotasRead = Permission{
		Name:   "mdb.quotas.read",
		Action: ASGet,
	}
	PermMDBAllSupport = Permission{
		Name:   "mdb.all.support",
		Action: ASUse,
	}
	PermMDBAllRead = Permission{
		Name:   "mdb.all.read",
		Action: ASGet,
	}
	PermMDBAllCreate = Permission{
		Name:   "mdb.all.create",
		Action: ASCreate,
	}
	PermMDBAllModify = Permission{
		Name:   "mdb.all.modify",
		Action: ASUpdate,
	}
	PermMDBAllDelete = NewPermissionWithForce("mdb.all.delete",
		ASDelete,
		&Permission{
			Name:   "mdb.all.forceDelete",
			Action: ASDelete,
		},
	)
	PermComputeHostGroupsUse = Permission{
		Name:   "compute.hostGroups.use",
		Action: ASUse,
	}
	PermVPCSubnetsUse = Permission{
		Name:   "vpc.subnets.use",
		Action: ASUse,
	}
	PermVPCAddressesCreateExternal = Permission{
		Name:   "vpc.addresses.createExternal",
		Action: ASCreate,
	}
	PermIAMServiceAccountsUse = Permission{
		Name:   "iam.serviceAccounts.use",
		Action: ASUse,
	}
	PermMDBClustersGetLogs = Permission{
		Name:   "mdb.clusters.getLogs",
		Action: ASGet,
	}
)

func (p Permission) IsGet() bool {
	return p.Action == ASGet
}

func (p Permission) IsCreate() bool {
	return p.Action == ASCreate
}

func (p Permission) WithForce() Permission {
	if p.withForce != nil {
		return *p.withForce
	} else {
		return p
	}
}
