package model

type ProductVersion struct {
	ID string

	Payload      Payload
	LicenseRules []RuleSpec
}

type Payload struct {
	ComputeImage *ComputeImageProductPayload
}

type ComputeImageProductPayload struct {
	ImageID  string
	FolderID string

	ResourceSpec ResourceSpec

	FormID      string
	PackageInfo PackageInfo

	// TODO: explanation.
	Codes []string

	ValidBaseImage bool

	PoolSize int // default = 1, minValue = 1
}

type PackageInfoContent struct {
	Name    string
	Version string
}

type PackageInfo struct {
	OS struct {
		Family  string
		Name    string
		Version string
	}

	Content []PackageInfoContent
}
