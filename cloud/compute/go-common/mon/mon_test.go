package mon

import (
	"context"
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
)

func TestRepository(t *testing.T) {
	assert := assert.New(t)

	const (
		testName  = "testname"
		okTest    = "okcheck"
		slowTest  = "slowcheck"
		panicTest = "paniccheck"
	)

	okFunc := func(ctx context.Context) error {
		return nil
	}

	slowFunc := func(ctx context.Context) error {
		deadline, _ := ctx.Deadline()
		time.Sleep(time.Until(deadline))
		time.Sleep(10 * time.Millisecond)
		return nil
	}

	panicFunc := func(ctx context.Context) error {
		panic("aaaaaaaaaa")
	}

	ctx := context.Background()

	repo := NewRepository(testName, WithTimeout(time.Millisecond*300))
	repo.Add(okTest, CheckFunc(okFunc))
	assert.Panics(func() {
		repo.Add(okTest, CheckFunc(okFunc))
	})

	st := repo.Run(ctx)
	assert.True(st.IsOK())
	wr := httptest.NewRecorder()
	req := httptest.NewRequest("GET", "/", nil)
	repo.ServeHTTP(wr, req)
	assert.Equal(http.StatusOK, wr.Code)
	assert.Equal(testName+`;0;{"okcheck":"Ok"}`, wr.Body.String())

	repo.Add(slowTest, CheckFunc(slowFunc))
	repo.Add(panicTest, CheckFunc(panicFunc))
	st = repo.Run(ctx)
	assert.False(st.IsOK())

	wr = httptest.NewRecorder()
	repo.ServeHTTP(wr, req)
	assert.Equal(http.StatusOK, wr.Code)
	fields := strings.Split(wr.Body.String(), ";")
	assert.Len(fields, 3)
	assert.Equal(testName, fields[0])
	assert.Equal("2", fields[1])

	var v = map[string]string{}
	assert.NoError(json.NewDecoder(strings.NewReader(fields[2])).Decode(&v))

	assert.Len(v, 3)
	assert.Equal(OK, v[okTest])
	assert.Equal(ErrExecutionTimeout.Error(), v[slowTest])
	assert.Equal(ErrCheckPanicked.Error(), v[panicTest])

	repo.Remove(slowTest)
	repo.Remove(panicTest)
	assert.True(repo.Run(ctx).IsOK())
}
