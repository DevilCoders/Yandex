package postgres_test

import (
	"context"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/cdn/cloud_api/pkg/storage"
	"a.yandex-team.ru/library/go/ptr"
)

func TestResourceRule(t *testing.T) {
	ctx := context.Background()

	db, err := constructCleanStorage()
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

	ruleEntity := resourceRuleEntity(createResource.EntityID, &groupEntity.EntityID, entity.EntityVersion)
	createResourceRule1, errorResult := db.CreateResourceRule(ctx, ruleEntity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	ruleEntity = resourceRuleEntity(createResource.EntityID, nil, entity.EntityVersion)
	createResourceRule2, errorResult := db.CreateResourceRule(ctx, ruleEntity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	rules, errorResult := db.GetAllRulesByResource(ctx, createResource.EntityID, nil)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	assert.Len(t, rules, 2)

	updateParams := updateRuleParams(createResourceRule1.EntityID, createResource.EntityID, int64(createResource.EntityVersion))
	_, errorResult = db.UpdateResourceRule(ctx, updateParams)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	ruleByID, errorResult := db.GetResourceRuleByID(ctx, createResourceRule1.EntityID, createResourceRule1.ResourceEntityID)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	assert.Equal(t, "name2", ruleByID.Name)

	errorResult = db.DeleteResourceRule(ctx, createResourceRule1.EntityID, createResourceRule1.ResourceEntityID)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	rules, errorResult = db.GetAllRulesByResource(ctx, createResource.EntityID, nil)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	if assert.Len(t, rules, 1) {
		assert.Equal(t, createResourceRule2.EntityID, rules[0].EntityID)
		assert.Equal(t, createResourceRule2.Name, rules[0].Name)
		assert.Equal(t, createResourceRule2.Pattern, rules[0].Pattern)
	}
}

func TestGetResourceRuleByVersion(t *testing.T) {
	ctx := context.Background()

	db, err := constructCleanStorage()
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

	resourceParams := updateResourceParams(createResource.EntityID, groupEntity.EntityID)
	updateResource, errorResult := db.UpdateResource(ctx, resourceParams)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	ruleEntity1 := resourceRuleEntity(createResource.EntityID, nil, createResource.EntityVersion)
	_, errorResult = db.CreateResourceRule(ctx, ruleEntity1)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	ruleEntity2 := resourceRuleEntity(updateResource.EntityID, nil, updateResource.EntityVersion)
	_, errorResult = db.CreateResourceRule(ctx, ruleEntity2)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	rules, errorResult := db.GetAllRulesByResource(ctx, ruleEntity1.ResourceEntityID, nil)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	if assert.Len(t, rules, 1) {
		assert.Equal(t, ruleEntity1.ResourceEntityVersion, rules[0].ResourceEntityVersion)
	}

	rules, errorResult = db.GetAllRulesByResource(ctx, ruleEntity2.ResourceEntityID, ptr.Int64(ruleEntity2.ResourceEntityVersion))
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}
	if assert.Len(t, rules, 1) {
		assert.Equal(t, ruleEntity2.ResourceEntityVersion, rules[0].ResourceEntityVersion)
	}
}

func TestCopyRulesWhenUpdateResource(t *testing.T) {
	ctx := context.Background()

	db, err := constructCleanStorage()
	if err != nil {
		t.Fatal(err)
	}

	// create origins group
	groupEntity := originsGroupEntity(150, true, nil)
	groupEntity, errorResult := db.CreateOriginsGroup(ctx, groupEntity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	// create resource
	entity := resourceEntity("entity_id", groupEntity.EntityID)
	createResource, errorResult := db.CreateResource(ctx, entity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	// create resource rule
	ruleEntity := resourceRuleEntity(createResource.EntityID, nil, createResource.EntityVersion)
	rule, errorResult := db.CreateResourceRule(ctx, ruleEntity)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	// update resource
	updateParams := updateResourceParams(createResource.EntityID, createResource.OriginsGroupEntityID)
	updateResource, errorResult := db.UpdateResource(ctx, updateParams)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	errorResult = db.ActivateResource(ctx, updateResource.EntityID, updateResource.EntityVersion)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	rules, errorResult := db.GetAllRulesByResource(ctx, updateResource.EntityID, nil)
	if errorResult != nil {
		t.Fatal(errorResult.Error())
	}

	if assert.Len(t, rules, 1) {
		assert.Equal(t, rule.ResourceEntityID, rules[0].ResourceEntityID)
		assert.Equal(t, rule.Name, rules[0].Name)
		assert.Equal(t, rule.Pattern, rules[0].Pattern)
		assert.EqualValues(t, updateResource.EntityVersion, rules[0].ResourceEntityVersion)
	}
}

func resourceRuleEntity(resourceID string, groupID *int64, version storage.EntityVersion) *storage.ResourceRuleEntity {
	return &storage.ResourceRuleEntity{
		EntityID:              storage.AutoGenID,
		ResourceEntityID:      resourceID,
		ResourceEntityVersion: int64(version),
		Name:                  "name",
		Pattern:               "pattern",
		OriginsGroupEntityID:  groupID,
		OriginProtocol:        nil,
		Options:               nil,
		CreatedAt:             storage.AutoGenTime,
		UpdatedAt:             storage.AutoGenTime,
		DeletedAt:             nil,
	}
}

func updateRuleParams(ruleID int64, resourceID string, resourceVersion int64) *storage.UpdateResourceRuleParams {
	return &storage.UpdateResourceRuleParams{
		ID:              ruleID,
		ResourceID:      resourceID,
		ResourceVersion: resourceVersion,
		Name:            "name2",
		Pattern:         "pattern2",
		Options:         nil,
	}
}
