package storage

import (
	"time"
)

//go:generate easyjson entity.go

const (
	AutoGenID = 0
)

var AutoGenTime time.Time

const (
	ResourceTable          = "resource_table"
	ResourceRuleTable      = "resource_rule_table"
	OriginsGroupTable      = "origins_group_table"
	OriginTable            = "origin_table"
	SecondaryHostnameTable = "secondary_hostnames_table"
)

type OriginsGroupEntity struct {
	RowID int64 `gorm:"primaryKey"`

	EntityID      int64
	EntityVersion EntityVersion
	EntityActive  bool

	FolderID string
	Name     string
	UseNext  bool
	Origins  []*OriginEntity `gorm:"foreignKey:OriginsGroupEntityID,OriginsGroupEntityVersion;references:EntityID,EntityVersion"`

	CreatedAt time.Time
	UpdatedAt time.Time
	DeletedAt *time.Time
}

func (e *OriginsGroupEntity) TableName() string {
	return OriginsGroupTable
}

type OriginEntity struct {
	EntityID                  int64 `gorm:"primaryKey"`
	OriginsGroupEntityID      int64
	OriginsGroupEntityVersion int64

	FolderID string
	Source   string
	Enabled  bool
	Backup   bool
	Type     OriginType

	CreatedAt time.Time
	UpdatedAt time.Time
	DeletedAt *time.Time
}

func (e *OriginEntity) TableName() string {
	return OriginTable
}

type ResourceEntity struct {
	RowID int64 `gorm:"primaryKey"`

	EntityID             string
	EntityVersion        EntityVersion
	EntityActive         bool
	OriginsGroupEntityID int64

	FolderID           string
	Active             bool
	Name               string
	Cname              string
	SecondaryHostnames []*SecondaryHostnameEntity `gorm:"foreignKey:ResourceEntityID,ResourceEntityVersion,ResourceEntityActive;references:EntityID,EntityVersion,EntityActive"`
	OriginProtocol     OriginProtocol

	Options *ResourceOptions `gorm:"embedded"`

	CreatedAt time.Time
	UpdatedAt time.Time
	DeletedAt *time.Time
}

func (e *ResourceEntity) TableName() string {
	return ResourceTable
}

type SecondaryHostnameEntity struct {
	RowID                 int64 `gorm:"primaryKey"`
	ResourceEntityID      string
	ResourceEntityVersion int64
	ResourceEntityActive  bool
	Hostname              string
}

func (e *SecondaryHostnameEntity) TableName() string {
	return SecondaryHostnameTable
}

type ResourceRuleEntity struct {
	EntityID              int64 `gorm:"primaryKey"`
	ResourceEntityID      string
	ResourceEntityVersion int64

	Name    string
	Pattern string

	OriginsGroupEntityID *int64
	OriginProtocol       *OriginProtocol

	Options *ResourceOptions `gorm:"embedded"`

	CreatedAt time.Time
	UpdatedAt time.Time
	DeletedAt *time.Time
}

func (e *ResourceRuleEntity) TableName() string {
	return ResourceRuleTable
}

type ResourceOptions struct {
	CustomHost      *string
	CustomSNI       *string
	RedirectToHTTPS *bool `gorm:"column:redirect_to_https"`
	AllowedMethods  AllowedMethodArray

	CORS                    *CORSOptions             `gorm:"embedded;embeddedPrefix:cors_"`
	BrowserCacheOptions     *BrowserCacheOptions     `gorm:"embedded;embeddedPrefix:browser_cache_"`
	EdgeCacheOptions        *EdgeCacheOptions        `gorm:"embedded;embeddedPrefix:edge_cache_"`
	ServeStaleOptions       *ServeStaleOptions       `gorm:"embedded;embeddedPrefix:serve_stale_"`
	NormalizeRequestOptions *NormalizeRequestOptions `gorm:"embedded;embeddedPrefix:normalize_request_"`
	CompressionOptions      *CompressionOptions      `gorm:"embedded;embeddedPrefix:compression_"`
	StaticHeadersOptions    *StaticHeadersOptions    `gorm:"embedded;embeddedPrefix:static_headers_"`
	RewriteOptions          *RewriteOptions          `gorm:"embedded;embeddedPrefix:rewrite_"`
}

type CORSOptions struct {
	Enabled        *bool
	EnableTiming   *bool
	Mode           *CORSMode
	AllowedOrigins StringArray
	AllowedMethods AllowedMethodArray
	AllowedHeaders StringArray
	MaxAge         *int64
	ExposeHeaders  StringArray
}

type BrowserCacheOptions struct {
	Enabled *bool
	MaxAge  *int64
}

type EdgeCacheOptions struct {
	Enabled      *bool
	UseRedirects *bool
	TTL          *int64
	Override     *bool

	OverrideTTLCodes OverrideTTLCodeArray
}

type OverrideTTLCode struct {
	Code int64
	TTL  int64
}

type ServeStaleOptions struct {
	Enabled *bool
	Errors  ServeStaleErrorArray
}

type NormalizeRequestOptions struct {
	Cookies     NormalizeRequestCookies `gorm:"embedded;embeddedPrefix:cookies_"`
	QueryString NormalizeRequestQueryString
}

type NormalizeRequestCookies struct {
	Ignore *bool
}

//easyjson:json
type NormalizeRequestQueryString struct { // one of
	Ignore    *bool       `json:"ignore,omitempty"`
	Whitelist StringArray `json:"whitelist,omitempty"`
	Blacklist StringArray `json:"blacklist,omitempty"`
}

type CompressionOptions struct {
	Variant CompressionVariant
}

//easyjson:json
type CompressionVariant struct { // one of
	FetchCompressed *bool     `json:"fetch_compressed,omitempty"`
	Compress        *Compress `json:"compress,omitempty"`
}

type Compress struct {
	Compress bool
	Codecs   CompressCodecArray
	Types    StringArray
}

type StaticHeadersOptions struct {
	Request  HeaderOptionArray
	Response HeaderOptionArray
}

type HeaderOption struct {
	Name   string
	Action HeaderAction
	Value  string
}

type RewriteOptions struct {
	Enabled     *bool
	Regex       *string
	Replacement *string
	Flag        *RewriteFlag
}

type UpdateOriginsGroupParams struct {
	ID      int64
	Name    string
	UseNext bool
	Origins []*OriginEntity
}

type GetAllOriginsGroupParams struct {
	FolderID       *string
	GroupIDs       *[]int64
	PreloadOrigins bool
	Page           *Pagination
}

type UpdateOriginParams struct {
	ID             int64
	OriginsGroupID int64
	FolderID       string
	Source         string
	Enabled        bool
	Backup         bool
	Type           OriginType
}

type UpdateResourceParams struct {
	ID                 string
	OriginsGroupID     int64
	Active             bool
	SecondaryHostnames *[]string
	OriginProtocol     *OriginProtocol
	Options            *ResourceOptions
}

type UpdateResourceRuleParams struct {
	ID                   int64
	ResourceID           string
	ResourceVersion      int64
	Name                 string
	Pattern              string
	OriginsGroupEntityID *int64
	OriginProtocol       *OriginProtocol
	Options              *ResourceOptions
}

type ResourceIDVersionPair struct {
	ResourceID      string
	ResourceVersion int64
}
