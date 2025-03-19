package ydblock

type Lock struct {
	Hosts       []string
	Deadline    uint64
	Description string
	Timeout     uint64
	Type        HostLockType
}
