package model

type OriginsGroup struct {
	ID       int64
	FolderID string
	Name     string
	UseNext  bool
	Origins  []*Origin

	Meta *VersionMeta
}

type Origin struct {
	ID       int64
	FolderID string
	Source   string
	Enabled  bool
	Backup   bool
	Type     OriginType
}

type CreateOriginParams struct {
	FolderID       string
	OriginsGroupID int64

	OriginParams
}

type GetOriginParams struct {
	OriginsGroupID int64
	OriginID       int64
}

type GetAllOriginParams struct {
	OriginsGroupID int64
}

type UpdateOriginParams struct {
	FolderID       string
	OriginsGroupID int64
	OriginID       int64

	OriginParams
}

type DeleteOriginParams struct {
	OriginsGroupID int64
	OriginID       int64
}

type OriginParams struct {
	Source  string
	Enabled bool
	Backup  bool
	Type    OriginType
}

type OriginType int64

const (
	OriginTypeCommon OriginType = iota
	OriginTypeBucket
	OriginTypeWebsite
)

type CreateOriginsGroupParams struct {
	FolderID string

	Name    string
	UseNext bool
	Origins []*OriginParams
}

type GetOriginsGroupParams struct {
	OriginsGroupID int64
}

type GetAllOriginsGroupParams struct {
	FolderID *string  // if not empty filtering by folderID
	GroupIDs *[]int64 // if not empty filtering by groupID
	Page     *Pagination
}

type UpdateOriginsGroupParams struct {
	OriginsGroupID int64

	Name    string
	UseNext bool
	Origins []*OriginParams
}

type DeleteOriginsGroupParams struct {
	OriginsGroupID int64
}

type ActivateOriginsGroupParams struct {
	OriginsGroupID int64
	Version        int64
}

type ListOriginsGroupVersionsParams struct {
	OriginsGroupID int64
	Versions       []int64
}
