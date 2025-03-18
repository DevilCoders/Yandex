package consoleapi

import (
	"google.golang.org/protobuf/types/known/wrapperspb"

	"a.yandex-team.ru/cdn/cloud_api/pkg/service/auth"
)

type FolderIDCache interface {
	Get(k interface{}) (interface{}, bool)
	Add(k, v interface{}) bool
}

func extractOptionalBool(v *wrapperspb.BoolValue, defaultValue bool) bool {
	if v != nil {
		return v.Value
	}
	return defaultValue
}

func makeOptionalFolderID(folderID string) *string {
	if folderID == "" {
		return nil
	}
	return &folderID
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
