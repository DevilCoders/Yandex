package ydblock

type HostLockType int64

const (
	// HostLockTypeOther any other workflow (update-cluster, reconfigure-balancers, etc.)
	HostLockTypeOther HostLockType = iota
	// HostLockTypeAddHosts add-host workflow
	HostLockTypeAddHosts
)

var HostLockTypeAll = []HostLockType{HostLockTypeAddHosts, HostLockTypeOther}

func (hlt HostLockType) String() string {
	switch hlt {
	case HostLockTypeOther:
		return "other"
	case HostLockTypeAddHosts:
		return "add-host"
	}
	return "unknown"
}

const (
	lockTable = "host_bootstrap_lock"
)
