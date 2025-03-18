package auth

import cloudauth "a.yandex-team.ru/cloud/iam/accessservice/client/iam-access-service-client-go/v1"

type AuthorizeResource interface {
	IsAuthorizeResource()
}

type CloudAuthResource interface {
	GetResources() []cloudauth.Resource
}

type Folder struct {
	FolderID string
}

func (f *Folder) IsAuthorizeResource() {}

func (f *Folder) GetResources() []cloudauth.Resource {
	return []cloudauth.Resource{cloudauth.ResourceFolder(f.FolderID)}
}

// TODO: not used
type Cloud struct {
	CloudID string
}

func (c *Cloud) IsAuthorizeResource() {}

func (c *Cloud) GetResources() []cloudauth.Resource {
	return []cloudauth.Resource{cloudauth.ResourceCloud(c.CloudID)}
}

type Entity struct {
	FolderID   string
	EntityID   string
	EntityType EntityType
}

func (e *Entity) IsAuthorizeResource() {}

func (e *Entity) GetResources() []cloudauth.Resource {
	return []cloudauth.Resource{
		{
			ID:   e.EntityID,
			Type: e.EntityType.String(),
		},
		cloudauth.ResourceFolder(e.FolderID),
	}
}

type EntityType string

func (t EntityType) String() string {
	return string(t)
}

const (
	CDNResourceType EntityType = "cdn.resource"
)
