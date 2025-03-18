package postgres_test

import (
	"context"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/cdn/cloud_api/pkg/errors"
	"a.yandex-team.ru/cdn/cloud_api/pkg/storage"
)

func TestOrigin(t *testing.T) {
	ctx := context.Background()

	db, err := constructCleanStorage()
	if err != nil {
		t.Fatal(err)
	}

	// init origins group
	groupEntity := originsGroupEntity(100, true, nil)
	originsGroup, errorResult := db.CreateOriginsGroup(ctx, groupEntity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	assert.Nil(t, originsGroup.Origins)

	originsGroupID := originsGroup.EntityID

	// create entity
	entity1 := originEntity(originsGroupID)
	createOrigin1, errorResult := db.CreateOrigin(ctx, entity1)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	assert.NotZero(t, createOrigin1.EntityID)
	assert.NotZero(t, createOrigin1.CreatedAt)
	assert.NotZero(t, createOrigin1.UpdatedAt)
	assert.Nil(t, createOrigin1.DeletedAt)
	assert.EqualValues(t, originsGroup.EntityVersion, createOrigin1.OriginsGroupEntityVersion)
	assert.NotEmpty(t, createOrigin1.EntityID)

	// create entity 2
	entity2 := originEntity(originsGroupID)
	createOrigin2, errorResult := db.CreateOrigin(ctx, entity2)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	assert.EqualValues(t, originsGroup.EntityVersion, createOrigin2.OriginsGroupEntityVersion)
	assert.NotEmpty(t, createOrigin2.EntityID)
	assert.NotEqual(t, createOrigin1.EntityID, createOrigin2.EntityID)

	// get origin
	getOriginByID, errorResult := db.GetOriginByID(ctx, originsGroupID, entity1.EntityID)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	assertEqualOrigins(t, createOrigin1, getOriginByID)

	// get all origins
	origins, errorResult := db.GetAllOrigins(ctx, originsGroupID)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	assert.Equal(t, 2, len(origins))

	// update origin
	params := updateOriginParams(createOrigin1.EntityID, originsGroupID)
	updateOrigin, errorResult := db.UpdateOrigin(ctx, params)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	assert.Equal(t, createOrigin1.EntityID, updateOrigin.EntityID)
	assert.Equal(t, "source2", updateOrigin.Source)
	assert.Equal(t, false, updateOrigin.Enabled)

	// delete origin
	errorResult = db.DeleteOriginByID(ctx, originsGroupID, createOrigin1.EntityID)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	// get all origins
	origins, errorResult = db.GetAllOrigins(ctx, originsGroupID)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	if assert.NotEmpty(t, origins) {
		assertEqualOrigins(t, createOrigin2, origins[0])
	}
}

func TestCreateOriginInActiveGroup(t *testing.T) {
	ctx := context.Background()

	db, err := constructCleanStorage()
	if err != nil {
		t.Fatal(err)
	}

	var errorResult errors.ErrorResult

	group1 := originsGroupEntity(10, true, nil)
	group1, errorResult = db.CreateOriginsGroup(ctx, group1)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	group2 := originsGroupEntity(15, false, nil)
	group2.EntityID = group1.EntityID
	group2, errorResult = db.CreateOriginsGroup(ctx, group2)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	origin := originEntity(group2.EntityID)
	createOrigin, errorResult := db.CreateOrigin(ctx, origin)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	assert.Equal(t, group1.EntityID, createOrigin.OriginsGroupEntityID)
	assert.EqualValues(t, group1.EntityVersion, createOrigin.OriginsGroupEntityVersion)

	// activate group2
	errorResult = db.ActivateOriginsGroup(ctx, group2.EntityID, group2.EntityVersion)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	origin2 := originEntity(group2.EntityID)
	createOrigin2, errorResult := db.CreateOrigin(ctx, origin2)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	assert.Equal(t, group2.EntityID, createOrigin2.OriginsGroupEntityID)
	assert.EqualValues(t, group2.EntityVersion, createOrigin2.OriginsGroupEntityVersion)
}

func assertEqualOrigins(t *testing.T, expected, actual *storage.OriginEntity) {
	assert.Equal(t, expected.EntityID, actual.EntityID)
	assert.Equal(t, expected.OriginsGroupEntityID, actual.OriginsGroupEntityID)
	assert.Equal(t, expected.OriginsGroupEntityVersion, actual.OriginsGroupEntityVersion)
	assert.Equal(t, expected.FolderID, actual.FolderID)
	assert.Equal(t, expected.Source, actual.Source)
	assert.Equal(t, expected.Enabled, actual.Enabled)
	assert.Equal(t, expected.Backup, actual.Backup)
	assert.Equal(t, expected.Type, actual.Type)
}

func originEntity(groupID int64) *storage.OriginEntity {
	return &storage.OriginEntity{
		EntityID:                  storage.AutoGenID,
		OriginsGroupEntityID:      groupID,
		OriginsGroupEntityVersion: 0,
		FolderID:                  folderID,
		Source:                    "source",
		Enabled:                   true,
		Backup:                    false,
		Type:                      storage.OriginTypeCommon,
		CreatedAt:                 storage.AutoGenTime,
		UpdatedAt:                 storage.AutoGenTime,
		DeletedAt:                 nil,
	}
}

func updateOriginParams(id int64, groupID int64) *storage.UpdateOriginParams {
	return &storage.UpdateOriginParams{
		ID:             id,
		OriginsGroupID: groupID,
		FolderID:       folderID,
		Source:         "source2",
		Enabled:        false,
		Backup:         false,
		Type:           storage.OriginTypeCommon,
	}
}
