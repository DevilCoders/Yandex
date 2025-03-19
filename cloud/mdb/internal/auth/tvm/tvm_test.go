package tvm_test

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/cloud/mdb/internal/auth/tvm"
)

func TestOAuthTokenFormat(t *testing.T) {
	inputs := []string{
		"abc",
		"146d8d54-368f-11e9-8035-8f3cbbd3a04c",
	}

	for _, token := range inputs {
		t.Run("Test "+token, func(t *testing.T) {
			assert.Equal(t, fmt.Sprintf("%s%s", tvm.OAuthTokenPrefix, token), tvm.FormatOAuthToken(token))
		})
	}
}

func TestOAuthTokenParse(t *testing.T) {
	inputs := []struct {
		oauth  string
		expect string
	}{
		{oauth: "OAuth abc", expect: "abc"},
		{oauth: "OAuth 146d8d54-368f-11e9-8035-8f3cbbd3a04c", expect: "146d8d54-368f-11e9-8035-8f3cbbd3a04c"},
		{oauth: "OAuth  abc ", expect: "abc"},
		{oauth: "OAuth  146d8d54-368f-11e9-8035-8f3cbbd3a04c ", expect: "146d8d54-368f-11e9-8035-8f3cbbd3a04c"},
		{oauth: " abc", expect: ""},
		{oauth: " 146d8d54-368f-11e9-8035-8f3cbbd3a04c", expect: ""},
		{oauth: "OAuth", expect: ""},
		{oauth: " OAuth abc ", expect: ""},
		{oauth: " OAuth 146d8d54-368f-11e9-8035-8f3cbbd3a04c ", expect: ""},
	}

	for _, input := range inputs {
		t.Run("Test "+input.oauth, func(t *testing.T) {
			parsed, err := tvm.ParseOAuthToken(input.oauth)
			if input.expect == "" {
				assert.Error(t, err)
				assert.Empty(t, parsed)
				return
			}

			assert.NoError(t, err)
			assert.Equal(t, input.expect, parsed)
		})
	}
}
