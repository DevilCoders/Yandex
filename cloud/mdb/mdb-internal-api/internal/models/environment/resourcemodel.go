package environment

type ResourceModel string

const (
	ResourceModelYandex    ResourceModel = "yandex"
	ResourceModelDataCloud ResourceModel = "datacloud"
)

var modelFolderMap = map[ResourceModel]string{
	ResourceModelYandex:    "folder",
	ResourceModelDataCloud: "project",
}

var modelCloudMap = map[ResourceModel]string{
	ResourceModelYandex:    "cloud",
	ResourceModelDataCloud: "project",
}

func (r ResourceModel) Cloud() string {
	return modelCloudMap[r]
}

func (r ResourceModel) Folder() string {
	return modelFolderMap[r]
}
