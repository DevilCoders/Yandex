package loader

import (
	"context"
	"errors"
	"fmt"
	"reflect"

	"github.com/heetch/confita"
	"github.com/heetch/confita/backend"
	"github.com/heetch/confita/backend/file"
	"github.com/imdario/mergo"
)

type MergableBackend interface {
	backend.Backend
	confita.Unmarshaler
}

type MergeBackend struct {
	MergableBackend
}

func MergeFiles(fn string) *MergeBackend {
	return &MergeBackend{MergableBackend: file.NewBackend(fn)}
}

func WithMergeLoad(backend MergableBackend) *MergeBackend {
	return &MergeBackend{MergableBackend: backend}
}

func (b *MergeBackend) Unmarshal(ctx context.Context, to interface{}) error {
	ref := reflect.ValueOf(to)

	if !ref.IsValid() || ref.Kind() != reflect.Ptr || ref.Elem().Kind() != reflect.Struct {
		return errors.New("provided target must be a pointer to struct")
	}

	toCopy := reflect.New(ref.Elem().Type()).Interface()

	if err := b.MergableBackend.Unmarshal(ctx, toCopy); err != nil {
		return err
	}

	if err := mergo.MergeWithOverwrite(to, toCopy); err != nil {
		return fmt.Errorf("merge error while unmarshal: %w", err)
	}

	return nil
}
