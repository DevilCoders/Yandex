package environment

import "time"

type Region struct {
	Name      string
	CloudType CloudType
	TimeZone  *time.Location
}

type Zone struct {
	Name      string
	RegionID  string
	CloudType CloudType
}
