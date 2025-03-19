package store

import (
	"a.yandex-team.ru/cloud/dataplatform/schemaregistry/internal/server/domain"
)

// Store is the interface that all database objects must implement.
type Store interface {
	domain.NamespaceRepository
	domain.SchemaRepository
}
