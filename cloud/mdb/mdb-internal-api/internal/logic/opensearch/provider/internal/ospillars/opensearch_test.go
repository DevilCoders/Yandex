package ospillars

import (
	"encoding/json"
	"testing"

	"github.com/stretchr/testify/require"
)

func TestKeepS3(t *testing.T) {
	data := json.RawMessage(`{
		"data": {
			"s3": {
				"some-key": "value",
				"other-key": "other-value"
			}
		}
	}`)

	data, err := unmarshalMarshal(data)
	require.NoError(t, err, "no errors expected on pillar unmarshal")

	var v struct {
		Data struct {
			S3 map[string]string `json:"s3"`
		} `json:"data"`
	}

	if err := json.Unmarshal(data, &v); err != nil {
		require.NoError(t, err, "should successfully decode json")
	}

	require.Equal(t, "value", v.Data.S3["some-key"], "should keep any s3 settings untouched")
	require.Equal(t, "other-value", v.Data.S3["other-key"], "should keep any s3 settings untouched")
}

func unmarshalMarshal(data json.RawMessage) (json.RawMessage, error) {
	p := NewCluster()
	if err := p.UnmarshalPillar(data); err != nil {
		return nil, err
	}

	return p.MarshalPillar()
}
