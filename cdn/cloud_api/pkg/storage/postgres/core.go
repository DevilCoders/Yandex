package postgres

import (
	"gorm.io/gorm"
)

const (
	star = "*"

	fieldRowID         = "row_id"
	fieldEntityID      = "entity_id"
	fieldEntityActive  = "entity_active"
	fieldEntityVersion = "entity_version"

	fieldFolderID                  = "folder_id"
	fieldOriginsGroupEntityID      = "origins_group_entity_id"
	fieldOriginsGroupEntityVersion = "origins_group_entity_version"
	fieldCreatedAt                 = "created_at"
	fieldUpdatedAt                 = "updated_at"
	fieldDeletedAt                 = "deleted_at"

	fieldResourceEntityID      = "resource_entity_id"
	fieldResourceEntityActive  = "resource_entity_active"
	fieldResourceEntityVersion = "resource_entity_version"

	preloadOriginsKey         = "Origins"
	preloadSecondaryHostnames = "SecondaryHostnames"
)

type Storage struct {
	db *gorm.DB
}

func NewStorage(db *gorm.DB) *Storage {
	return &Storage{
		db: db,
	}
}
