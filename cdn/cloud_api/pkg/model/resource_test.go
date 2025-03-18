package model

import (
	"testing"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

func TestSetResourceOptionsDefaults(t *testing.T) {
	var options *ResourceOptions
	newOptions := SetResourceOptionsDefaults(options)

	require.NotNil(t, newOptions)
	require.NoError(t, newOptions.Validate())

	assert.NotZero(t, len(newOptions.AllowedMethods))
	assert.NotNil(t, newOptions.EdgeCacheOptions.Enabled)
	assert.NotNil(t, newOptions.NormalizeRequestOptions.Cookies.Ignore)
	assert.NotNil(t, newOptions.CompressionOptions.Variant.Compress)
	assert.NotZero(t, len(newOptions.CompressionOptions.Variant.Compress.Codecs))
}
