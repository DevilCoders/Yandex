package model

type ResourceSpec struct {
	CPU               ResourceIntervalRestriction
	CPUFraction       ResourceIntervalRestriction
	Memory            ResourceIntervalRestriction
	GPU               ResourceIntervalRestriction
	DiskSize          ResourceIntervalRestriction
	NetworkInterfaces ResourceIntervalRestriction

	ServiceAccountsRoles []string
	ComputePlatforms     []string

	LicenseInstancePool string
}

type ResourceIntervalRestriction struct {
	Min int64
	Max int64

	Recommended int64
}

func (r ResourceIntervalRestriction) Empty() bool {
	return r.Max == 0 && r.Min == 0 && r.Recommended == 0
}
