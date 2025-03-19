package models

type MDBClusterSpec struct {
	Version           string
	SubnetIDs         []string
	MinServersPerZone int64
	MaxServersPerZone int64
	NetworkID         string
	EndpointIP        string
}

type ClusterConfigSpec struct {
	ZoneID []string
}

func (cs ClusterConfigSpec) Validate() error {
	return nil
}
