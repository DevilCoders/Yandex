package postgres_test

import (
	"context"
	"fmt"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/cdn/cloud_api/pkg/storage"
)

func TestEraseSoftDeletedResource(t *testing.T) {
	ctx := context.Background()

	db, err := constructCleanStorage()
	if err != nil {
		t.Fatal(err)
	}

	gorm, err := constructGorm()
	if err != nil {
		t.Fatal(err)
	}

	groupEntity := originsGroupEntity(150, true, nil)
	groupEntity, errorResult := db.CreateOriginsGroup(ctx, groupEntity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	entity := resourceEntity("entity_id", groupEntity.EntityID)
	entity.SecondaryHostnames = secondaryHostnames([]string{"1", "2"})
	createResource, errorResult := db.CreateResource(ctx, entity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	ruleEntity := resourceRuleEntity(createResource.EntityID, &createResource.OriginsGroupEntityID, createResource.EntityVersion)
	_, errorResult = db.CreateResourceRule(ctx, ruleEntity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	params := updateResourceParams(createResource.EntityID, createResource.OriginsGroupEntityID)
	_, errorResult = db.UpdateResource(ctx, params)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	errorResult = db.DeleteResourceByID(ctx, createResource.EntityID)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	result := gorm.Exec(fmt.Sprintf("SELECT * FROM %s", storage.ResourceTable))
	if result.Error != nil {
		t.Fatal(result.Error)
	}
	assert.EqualValues(t, 2, result.RowsAffected)

	result = gorm.Exec(fmt.Sprintf("SELECT * FROM %s", storage.ResourceRuleTable))
	if result.Error != nil {
		t.Fatal(result.Error)
	}
	assert.EqualValues(t, 2, result.RowsAffected)

	result = gorm.Exec(fmt.Sprintf("SELECT * FROM %s", storage.SecondaryHostnameTable))
	if result.Error != nil {
		t.Fatal(result.Error)
	}
	assert.EqualValues(t, 3, result.RowsAffected)

	errorResult = db.EraseSoftDeletedResource(ctx, time.Now())
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	result = gorm.Exec(fmt.Sprintf("SELECT * FROM %s", storage.ResourceTable))
	if result.Error != nil {
		t.Fatal(result.Error)
	}
	assert.EqualValues(t, 0, result.RowsAffected)

	result = gorm.Exec(fmt.Sprintf("SELECT * FROM %s", storage.ResourceRuleTable))
	if result.Error != nil {
		t.Fatal(result.Error)
	}
	assert.EqualValues(t, 0, result.RowsAffected)

	result = gorm.Exec(fmt.Sprintf("SELECT * FROM %s", storage.SecondaryHostnameTable))
	if result.Error != nil {
		t.Fatal(result.Error)
	}
	assert.EqualValues(t, 0, result.RowsAffected)
}

func TestEraseOldResourceVersions(t *testing.T) {
	ctx := context.Background()

	db, err := constructCleanStorage()
	if err != nil {
		t.Fatal(err)
	}

	gorm, err := constructGorm()
	if err != nil {
		t.Fatal(err)
	}

	groupEntity := originsGroupEntity(150, true, nil)
	groupEntity, errorResult := db.CreateOriginsGroup(ctx, groupEntity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	entity := resourceEntity("entity_id", groupEntity.EntityID)
	createResource, errorResult := db.CreateResource(ctx, entity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	ruleEntity := resourceRuleEntity(createResource.EntityID, &createResource.OriginsGroupEntityID, createResource.EntityVersion)
	_, errorResult = db.CreateResourceRule(ctx, ruleEntity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	params := updateResourceParams(createResource.EntityID, createResource.OriginsGroupEntityID)
	for i := 0; i < 10; i++ {
		updateResource, errorResult := db.UpdateResource(ctx, params)
		if errorResult != nil {
			t.Fatal(errorResult.Error())
		}

		errorResult = db.ActivateResource(ctx, updateResource.EntityID, updateResource.EntityVersion)
		if errorResult != nil {
			t.Fatal(errorResult.Error())
		}
	}

	result := gorm.Exec(fmt.Sprintf("SELECT * FROM %s", storage.ResourceTable))
	if result.Error != nil {
		t.Fatal(result.Error)
	}
	assert.EqualValues(t, 11, result.RowsAffected)

	result = gorm.Exec(fmt.Sprintf("SELECT * FROM %s", storage.ResourceRuleTable))
	if result.Error != nil {
		t.Fatal(result.Error)
	}
	assert.EqualValues(t, 11, result.RowsAffected)

	errorResult = db.EraseOldResourceVersions(ctx, createResource.EntityID, 11-5)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	result = gorm.Exec(fmt.Sprintf("SELECT * FROM %s", storage.ResourceTable))
	if result.Error != nil {
		t.Fatal(result.Error)
	}
	assert.EqualValues(t, 5, result.RowsAffected)

	result = gorm.Exec(fmt.Sprintf("SELECT * FROM %s", storage.ResourceRuleTable))
	if result.Error != nil {
		t.Fatal(result.Error)
	}
	assert.EqualValues(t, 5, result.RowsAffected)
}

func TestEraseSoftDeletedOriginsGroup(t *testing.T) {
	ctx := context.Background()

	db, err := constructCleanStorage()
	if err != nil {
		t.Fatal(err)
	}

	gorm, err := constructGorm()
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

	params := updateGroupParams(originsGroup.EntityID, []*storage.OriginEntity{
		originEntity(0),
	})
	updatedGroup, errorResult := db.UpdateOriginsGroup(ctx, params)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	errorResult = db.DeleteOriginsGroupByID(ctx, updatedGroup.EntityID)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	result := gorm.Exec(fmt.Sprintf("SELECT * FROM %s", storage.OriginsGroupTable))
	if result.Error != nil {
		t.Fatal(result.Error)
	}
	assert.EqualValues(t, 2, result.RowsAffected)

	result = gorm.Exec(fmt.Sprintf("SELECT * FROM %s", storage.OriginTable))
	if result.Error != nil {
		t.Fatal(result.Error)
	}
	assert.EqualValues(t, 4, result.RowsAffected)

	errorResult = db.EraseSoftDeletedOriginsGroup(ctx, time.Now())
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	result = gorm.Exec(fmt.Sprintf("SELECT * FROM %s", storage.OriginsGroupTable))
	if result.Error != nil {
		t.Fatal(result.Error)
	}
	assert.EqualValues(t, 0, result.RowsAffected)

	result = gorm.Exec(fmt.Sprintf("SELECT * FROM %s", storage.OriginTable))
	if result.Error != nil {
		t.Fatal(result.Error)
	}
	assert.EqualValues(t, 0, result.RowsAffected)
}

func TestEraseOldOriginsGroupVersions(t *testing.T) {
	ctx := context.Background()

	db, err := constructCleanStorage()
	if err != nil {
		t.Fatal(err)
	}

	gorm, err := constructGorm()
	if err != nil {
		t.Fatal(err)
	}

	entity := originsGroupEntity(1, true, []*storage.OriginEntity{
		originEntity(0),
		originEntity(0),
		originEntity(0),
	})
	originsGroup, errorResult := db.CreateOriginsGroup(ctx, entity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	for i := 0; i < 10; i++ {
		params := updateGroupParams(originsGroup.EntityID, []*storage.OriginEntity{
			originEntity(0),
		})
		_, errorResult = db.UpdateOriginsGroup(ctx, params)
		if errorResult != nil {
			t.Fatal(errorResult.Error())
		}
	}

	result := gorm.Exec(fmt.Sprintf("SELECT * FROM %s", storage.OriginsGroupTable))
	if result.Error != nil {
		t.Fatal(result.Error)
	}
	assert.EqualValues(t, 11, result.RowsAffected)

	result = gorm.Exec(fmt.Sprintf("SELECT * FROM %s", storage.OriginTable))
	if result.Error != nil {
		t.Fatal(result.Error)
	}
	assert.EqualValues(t, 13, result.RowsAffected)

	errorResult = db.EraseOldOriginsGroupVersions(ctx, originsGroup.EntityID, 11-5)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	result = gorm.Exec(fmt.Sprintf("SELECT * FROM %s", storage.OriginsGroupTable))
	if result.Error != nil {
		t.Fatal(result.Error)
	}
	assert.EqualValues(t, 5, result.RowsAffected)

	result = gorm.Exec(fmt.Sprintf("SELECT * FROM %s", storage.OriginTable))
	if result.Error != nil {
		t.Fatal(result.Error)
	}
	assert.EqualValues(t, 5, result.RowsAffected)
}
