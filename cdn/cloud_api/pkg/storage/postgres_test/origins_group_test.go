package postgres_test

import (
	"context"
	"math/rand"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/cdn/cloud_api/pkg/storage"
	"a.yandex-team.ru/library/go/ptr"
)

const (
	folderID = "folder_id"
	name     = "name"
)

func TestOriginsGroup(t *testing.T) {
	ctx := context.Background()

	db, err := constructCleanStorage()
	if err != nil {
		t.Fatal(err)
	}

	// create entity
	entity := originsGroupEntity(10, true, nil)
	createEntity, errorResult := db.CreateOriginsGroup(ctx, entity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	assert.NotZero(t, createEntity.RowID)
	assert.NotZero(t, createEntity.CreatedAt)
	assert.NotZero(t, createEntity.UpdatedAt)
	assert.Nil(t, createEntity.DeletedAt)
	assertEqualGroups(t, entity, createEntity)

	// get entities by folder id
	getAllOriginsGroupsByFolderID, errorResult := db.GetAllOriginsGroups(ctx, &storage.GetAllOriginsGroupParams{
		FolderID:       ptr.String(folderID),
		PreloadOrigins: false,
		Page:           page,
	})
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	if assert.NotEmpty(t, getAllOriginsGroupsByFolderID) {
		assertEqualGroups(t, createEntity, getAllOriginsGroupsByFolderID[0])
	}

	// get entities
	getAllOriginsGroups, errorResult := db.GetAllOriginsGroups(ctx, &storage.GetAllOriginsGroupParams{
		PreloadOrigins: false,
		Page:           page,
	})
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	if assert.NotEmpty(t, getAllOriginsGroups) {
		assertEqualGroups(t, createEntity, getAllOriginsGroups[0])
	}

	// update entity
	params := updateGroupParams(createEntity.EntityID, nil)
	updateOriginsGroup, errorResult := db.UpdateOriginsGroup(ctx, params)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	assert.Equal(t, createEntity.EntityVersion+1, updateOriginsGroup.EntityVersion)
	assert.Equal(t, false, updateOriginsGroup.EntityActive)
	assert.Equal(t, createEntity.FolderID, updateOriginsGroup.FolderID)
	assert.Equal(t, createEntity.Name+"2", updateOriginsGroup.Name)
	assert.Equal(t, false, updateOriginsGroup.UseNext)

	// get old entity
	getEntity, errorResult := db.GetOriginsGroupByID(ctx, entity.EntityID, false)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	assertEqualGroups(t, createEntity, getEntity)

	// activate entity
	errorResult = db.ActivateOriginsGroup(ctx, updateOriginsGroup.EntityID, updateOriginsGroup.EntityVersion)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	// get new entity (updated)
	newOriginsGroup, errorResult := db.GetOriginsGroupByID(ctx, updateOriginsGroup.EntityID, false)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	assert.Equal(t, createEntity.EntityVersion+1, newOriginsGroup.EntityVersion)
	assert.Equal(t, true, newOriginsGroup.EntityActive)
	assert.Equal(t, createEntity.FolderID, newOriginsGroup.FolderID)
	assert.Equal(t, createEntity.Name+"2", newOriginsGroup.Name)
	assert.Equal(t, false, newOriginsGroup.UseNext)

	// delete entity
	errorResult = db.DeleteOriginsGroupByID(ctx, createEntity.EntityID)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	groups, errorResult := db.GetAllOriginsGroups(ctx, &storage.GetAllOriginsGroupParams{
		PreloadOrigins: false,
		Page:           page,
	})
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	assert.Empty(t, groups)
}

func TestDeleteOriginsGroup(t *testing.T) {
	ctx := context.Background()

	db, err := constructCleanStorage()
	if err != nil {
		t.Fatal(err)
	}

	groupEntity := originsGroupEntity(30, true, nil)
	createdGroup, errorResult := db.CreateOriginsGroup(ctx, groupEntity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	resource := resourceEntity("entity_id", createdGroup.EntityID)
	createdResource, errorResult := db.CreateResource(ctx, resource)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	// attempt to delete origins group
	errorResult = db.DeleteOriginsGroupByID(ctx, createdGroup.EntityID)
	assert.NotNil(t, errorResult)

	getOriginsGroupByID, errorResult := db.GetOriginsGroupByID(ctx, createdGroup.EntityID, false)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	assertEqualGroups(t, createdGroup, getOriginsGroupByID)

	// delete resource and origins group
	errorResult = db.DeleteResourceByID(ctx, createdResource.EntityID)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	errorResult = db.DeleteOriginsGroupByID(ctx, createdGroup.EntityID)
	assert.Nil(t, errorResult)
}

func TestDeleteOriginsForAllGroupVersion(t *testing.T) {
	ctx := context.Background()

	db, err := constructCleanStorage()
	if err != nil {
		t.Fatal(err)
	}

	entity := originsGroupEntity(1, true, []*storage.OriginEntity{
		originEntity(0),
		originEntity(0),
	})
	createdGroup, errorResult := db.CreateOriginsGroup(ctx, entity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	params := updateGroupParams(createdGroup.EntityID, []*storage.OriginEntity{originEntity(0)})
	updatedGroup, errorResult := db.UpdateOriginsGroup(ctx, params)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	errorResult = db.DeleteOriginsGroupByID(ctx, updatedGroup.EntityID)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	for _, origin := range createdGroup.Origins {
		_, errorResult = db.GetOriginByID(ctx, createdGroup.EntityID, origin.EntityID)
		assert.NotNil(t, errorResult)
	}
}

func TestGetAllGroups(t *testing.T) {
	ctx := context.Background()

	db, err := constructCleanStorage()
	if err != nil {
		t.Fatal(err)
	}

	entity1 := originsGroupEntity(10, true, nil)
	entity1.FolderID = "1"
	entity1, errorResult := db.CreateOriginsGroup(ctx, entity1)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	entity2 := originsGroupEntity(1, true, nil)
	entity2.FolderID = "2"
	entity2, errorResult = db.CreateOriginsGroup(ctx, entity2)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	groups, errorResult := db.GetAllOriginsGroups(ctx, &storage.GetAllOriginsGroupParams{
		PreloadOrigins: false,
		Page:           page,
	})
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	assert.Equal(t, 2, len(groups))

	groups, errorResult = db.GetAllOriginsGroups(ctx, &storage.GetAllOriginsGroupParams{
		FolderID:       ptr.String("1"),
		PreloadOrigins: false,
		Page:           page,
	})
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	if assert.NotEmpty(t, groups) {
		assert.Equal(t, "1", groups[0].FolderID)
		assert.Equal(t, entity1.EntityID, groups[0].EntityID)
	}

	groups, errorResult = db.GetAllOriginsGroups(ctx, &storage.GetAllOriginsGroupParams{
		FolderID:       ptr.String("2"),
		PreloadOrigins: false,
		Page:           page,
	})
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	if assert.NotEmpty(t, groups) {
		assert.Equal(t, "2", groups[0].FolderID)
		assert.Equal(t, entity2.EntityID, groups[0].EntityID)
	}
}

func TestOriginsInGroup(t *testing.T) {
	ctx := context.Background()

	db, err := constructCleanStorage()
	if err != nil {
		t.Fatal(err)
	}

	entity := originsGroupEntity(10, true, []*storage.OriginEntity{
		originEntity(0),
		originEntity(0),
		originEntity(0),
	})
	originsGroup, errorResult := db.CreateOriginsGroup(ctx, entity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	origins, errorResult := db.GetAllOrigins(ctx, originsGroup.EntityID)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	assert.Equal(t, len(originsGroup.Origins), len(origins))

	params := updateGroupParams(originsGroup.EntityID, []*storage.OriginEntity{
		originEntity(0),
	})
	updatedGroup, errorResult := db.UpdateOriginsGroup(ctx, params)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	errorResult = db.ActivateOriginsGroup(ctx, updatedGroup.EntityID, updatedGroup.EntityVersion)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	origins, errorResult = db.GetAllOrigins(ctx, updatedGroup.EntityID)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	assert.Equal(t, len(updatedGroup.Origins), len(origins))
}

func TestUpdatedGroupCreatedTime(t *testing.T) {
	ctx := context.Background()

	db, err := constructCleanStorage()
	if err != nil {
		t.Fatal(err)
	}

	// create entity
	entity := originsGroupEntity(10, true, nil)
	createEntity, errorResult := db.CreateOriginsGroup(ctx, entity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	// update entity
	params := updateGroupParams(createEntity.EntityID, nil)
	updateOriginsGroup, errorResult := db.UpdateOriginsGroup(ctx, params)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	assertTime(t, createEntity.CreatedAt, updateOriginsGroup.CreatedAt)
}

func TestOriginsGroupRepeatedActivate(t *testing.T) {
	ctx := context.Background()

	db, err := constructCleanStorage()
	if err != nil {
		t.Fatal(err)
	}

	groupEntity := originsGroupEntity(30, true, nil)
	groupEntity.EntityActive = false
	createdGroup, errorResult := db.CreateOriginsGroup(ctx, groupEntity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	errorResult = db.ActivateOriginsGroup(ctx, createdGroup.EntityID, createdGroup.EntityVersion)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	groupByID1, errorResult := db.GetOriginsGroupByID(ctx, createdGroup.EntityID, false)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	createdGroup.EntityActive = true
	assertEqualGroups(t, createdGroup, groupByID1)

	errorResult = db.ActivateOriginsGroup(ctx, createdGroup.EntityID, createdGroup.EntityVersion)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	groupByID2, errorResult := db.GetOriginsGroupByID(ctx, createdGroup.EntityID, false)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	assertEqualGroups(t, createdGroup, groupByID2)

	assertEqualGroups(t, groupByID1, groupByID2)
}

func TestFilterGroupsByIDs(t *testing.T) {
	ctx := context.Background()

	db, err := constructCleanStorage()
	if err != nil {
		t.Fatal(err)
	}

	var ids []int64

	for i := 0; i < 10; i++ {
		groupEntity := originsGroupEntity(1, true, nil)
		createdGroup, errorResult := db.CreateOriginsGroup(ctx, groupEntity)
		if errorResult != nil {
			t.Fatal(errorResult.Error())
		}
		ids = append(ids, createdGroup.EntityID)
	}

	sourceGroups, errorResult := db.GetAllOriginsGroups(ctx, &storage.GetAllOriginsGroupParams{
		GroupIDs: nil,
	})
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	assert.Len(t, sourceGroups, 10)

	sourceGroups, errorResult = db.GetAllOriginsGroups(ctx, &storage.GetAllOriginsGroupParams{
		GroupIDs: &ids,
	})
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	assert.Len(t, sourceGroups, 10)

	reqIDs := ids[0:5]
	groups, errorResult := db.GetAllOriginsGroups(ctx, &storage.GetAllOriginsGroupParams{
		GroupIDs: &reqIDs,
	})
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	assert.Len(t, groups, 5)

	for i := 0; i < len(groups); i++ {
		assertEqualGroups(t, sourceGroups[i], groups[i])
	}
}

func assertEqualGroups(t *testing.T, expected, actual *storage.OriginsGroupEntity) {
	assert.Equal(t, expected.RowID, actual.RowID)
	assert.Equal(t, expected.EntityID, actual.EntityID)
	assert.Equal(t, expected.EntityVersion, actual.EntityVersion)
	assert.Equal(t, expected.EntityActive, actual.EntityActive)
	assert.Equal(t, expected.FolderID, actual.FolderID)
	assert.Equal(t, expected.Name, actual.Name)
	assert.Equal(t, expected.UseNext, actual.UseNext)
	assert.Equal(t, expected.Origins, actual.Origins)
}

func originsGroupEntity(version int64, active bool, origins []*storage.OriginEntity) *storage.OriginsGroupEntity {
	rand.Seed(time.Now().UnixNano())

	return &storage.OriginsGroupEntity{
		RowID:         storage.AutoGenID,
		EntityID:      rand.Int63(),
		EntityVersion: storage.EntityVersion(version),
		EntityActive:  active,
		FolderID:      folderID,
		Name:          name,
		UseNext:       true,
		Origins:       origins,
		CreatedAt:     storage.AutoGenTime,
		UpdatedAt:     storage.AutoGenTime,
		DeletedAt:     nil,
	}
}

func updateGroupParams(id int64, origins []*storage.OriginEntity) *storage.UpdateOriginsGroupParams {
	return &storage.UpdateOriginsGroupParams{
		ID:      id,
		Name:    name + "2",
		UseNext: false,
		Origins: origins,
	}
}
