package userapi

import "a.yandex-team.ru/cdn/cloud_api/pkg/service/auth"

type FolderIDCache interface {
	Get(k interface{}) (interface{}, bool)
	Add(k, v interface{}) bool
}

func extractInt64(i *int64) int64 {
	if i != nil {
		return *i
	}
	return 0
}

func folder(folderID string) *auth.Folder {
	return &auth.Folder{
		FolderID: folderID,
	}
}

func entityResource(folderID, entityID string) *auth.Entity {
	return &auth.Entity{
		FolderID:   folderID,
		EntityID:   entityID,
		EntityType: auth.CDNResourceType,
	}
}
