package mapper

import (
	"a.yandex-team.ru/cdn/cloud_api/pkg/model"
	pbmodel "a.yandex-team.ru/cdn/cloud_api/proto/model"
)

func MakePBOriginsGroups(groups []*model.OriginsGroup) []*pbmodel.OriginsGroup {
	result := make([]*pbmodel.OriginsGroup, 0, len(groups))
	for _, group := range groups {
		result = append(result, MakePBOriginsGroup(group))
	}

	return result
}

func MakePBOriginsGroup(group *model.OriginsGroup) *pbmodel.OriginsGroup {
	if group == nil {
		return nil
	}

	return &pbmodel.OriginsGroup{
		Id:          group.ID,
		FolderId:    group.FolderID,
		Name:        group.Name,
		UseNext:     group.UseNext,
		Origins:     MakePBOrigins(group.Origins),
		VersionMeta: makeEntityMeta(group.Meta),
	}
}

func MakePBOrigins(origins []*model.Origin) []*pbmodel.Origin {
	result := make([]*pbmodel.Origin, 0, len(origins))
	for _, origin := range origins {
		result = append(result, MakePBOrigin(origin))
	}

	return result
}

func MakePBOrigin(origin *model.Origin) *pbmodel.Origin {
	if origin == nil {
		return nil
	}

	return &pbmodel.Origin{
		Id:       origin.ID,
		FolderId: origin.FolderID,
		Source:   origin.Source,
		Enabled:  origin.Enabled,
		Backup:   origin.Backup,
		Type:     makePBOriginType(origin.Type),
	}
}

func makePBOriginType(originType model.OriginType) pbmodel.OriginType {
	switch originType {
	case model.OriginTypeCommon:
		return pbmodel.OriginType_COMMON
	case model.OriginTypeBucket:
		return pbmodel.OriginType_BUCKET
	case model.OriginTypeWebsite:
		return pbmodel.OriginType_WEBSITE
	default:
		return pbmodel.OriginType_ORIGIN_TYPE_UNSPECIFIED
	}
}
