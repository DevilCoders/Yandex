package pgmodels

type Database struct {
	Name      string
	ClusterID string
	Owner     string
	LCCollate string
	LCCtype   string
	// TODO: extensions
	// Extensions []*Extension
}
