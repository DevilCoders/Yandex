package request

import (
	"context"
	"sync"

	"a.yandex-team.ru/library/go/core/log"
)

type Attributes struct {
	mu sync.Mutex

	userID   *string
	userType *string
	folderID *string
	cloudID  *string
}

func SetRequestUserID(ctx context.Context, userID string) {
	if ra, ok := attributes(ctx); ok {
		ra.mu.Lock()
		defer ra.mu.Unlock()
		ra.userID = &userID
	}
}

func SetRequestUserType(ctx context.Context, userType string) {
	if ra, ok := attributes(ctx); ok {
		ra.mu.Lock()
		defer ra.mu.Unlock()
		ra.userType = &userType
	}
}

func SetRequestFolderID(ctx context.Context, folderID string) {
	if ra, ok := attributes(ctx); ok {
		ra.mu.Lock()
		defer ra.mu.Unlock()
		ra.folderID = &folderID
	}
}

func SetRequestCloudID(ctx context.Context, cloudID string) {
	if ra, ok := attributes(ctx); ok {
		ra.mu.Lock()
		defer ra.mu.Unlock()
		ra.cloudID = &cloudID
	}
}

type raKey struct{}

func WithAttributes(ctx context.Context) context.Context {
	return context.WithValue(ctx, raKey{}, &Attributes{})
}

func attributes(ctx context.Context) (*Attributes, bool) {
	ra := ctx.Value(raKey{})
	if ra == nil {
		return nil, false
	}
	v, _ := ra.(*Attributes)
	return v, true
}

func LogFields(ctx context.Context) []log.Field {
	if ra, ok := attributes(ctx); ok {
		ra.mu.Lock()
		defer ra.mu.Unlock()
		fields := make([]log.Field, 0, 2)
		if ra.userID != nil {
			fields = append(fields, log.String("user_id", *ra.userID))
		}
		if ra.userType != nil {
			fields = append(fields, log.String("user_type", *ra.userType))
		}
		if ra.cloudID != nil {
			fields = append(fields, log.String("cloud.ext_id", *ra.cloudID))
		}
		if ra.folderID != nil {
			fields = append(fields, log.String("folder.ext_id", *ra.folderID))
		}
		return fields
	} else {
		return nil
	}
}

func SentryTags(ctx context.Context) map[string]string {
	tags := make(map[string]string)
	if ra, ok := attributes(ctx); ok {
		ra.mu.Lock()
		defer ra.mu.Unlock()
		if ra.userID != nil {
			tags["user_id"] = *ra.userID
		}
		if ra.userType != nil {
			tags["user_type"] = *ra.userType
		}
		if ra.cloudID != nil {
			tags["cloud.id"] = *ra.cloudID
		}
		if ra.folderID != nil {
			tags["folder.id"] = *ra.folderID
		}
	}
	return tags
}
