package mapper

import (
	"a.yandex-team.ru/cdn/cloud_api/pkg/model"
	pbmodel "a.yandex-team.ru/cdn/cloud_api/proto/model"
)

func makeEntityMeta(meta *model.VersionMeta) *pbmodel.VersionMeta {
	if meta == nil {
		return nil
	}

	return &pbmodel.VersionMeta{
		Version: meta.Version,
		Active:  meta.Active,
	}
}
