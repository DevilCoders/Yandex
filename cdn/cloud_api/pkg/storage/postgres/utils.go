package postgres

import "gorm.io/gorm/clause"

func gormColumn(name string) clause.Column {
	return clause.Column{
		Name: name,
	}
}

type filterOptions func(map[string]interface{})

func filterFolderID(folderID string) func(map[string]interface{}) {
	return func(m map[string]interface{}) {
		m[fieldFolderID] = folderID
	}
}

func filterEntityIDs(ids interface{}) filterOptions {
	return func(m map[string]interface{}) {
		m[fieldEntityID] = ids
	}
}

func filterVersions(versions []int64) func(map[string]interface{}) {
	return func(m map[string]interface{}) {
		m[fieldEntityVersion] = versions
	}
}
