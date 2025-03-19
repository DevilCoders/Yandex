package main

import (
	"a.yandex-team.ru/library/go/core/metrics/solomon"
	"encoding/json"
	"github.com/google/go-cmp/cmp"
	"github.com/stretchr/testify/assert"
	"testing"
)

func Test_prepareData(t *testing.T) {
	testCases := []struct {
		name      string
		registry  *solomon.Registry
		host      string
		gaugeName string

		expect    string
		expectErr error
	}{
		{
			"success",
			func() *solomon.Registry {
				//Cloud Solomon expects "name" label inside metric aswell
				regOpts := solomon.RegistryOpts{
					Tags: map[string]string{"name": `ru-central1-a`},
				}

				r := solomon.NewRegistry(&regOpts)

				return r
			}(),
			"ru-central1-a",
			"kms_available",
			`{"commonLabels":{"host":"ru-central1-a"},"metrics":[{"labels":{"name":"ru-central1-a","sensor":"kms_available"},"type":"DGAUGE","value":1}]}`,
			nil,
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			gauge := tc.registry.Gauge(tc.gaugeName)
			gauge.Set(1)
			data, err := prepareData(tc.host, *tc.registry, gauge)

			if tc.expectErr == nil {
				assert.NoError(t, err)
			} else {
				assert.EqualError(t, err, tc.expectErr.Error())
			}

			assert.Equal(t, len(tc.expect), len(data))

			if tc.expect != "" {
				var expectedObj, givenObj map[string]interface{}
				err = json.Unmarshal([]byte(tc.expect), &expectedObj)
				assert.NoError(t, err)
				err = json.Unmarshal(data, &givenObj)
				assert.NoError(t, err)

				assert.True(t, cmp.Equal(expectedObj, givenObj), cmp.Diff(expectedObj, givenObj))
			}
		})
	}
}
