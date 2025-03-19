package http

import (
	"net/http"
	"net/http/httptest"
	"testing"
	"time"

	"a.yandex-team.ru/cloud/ps/gore/pkg/abc"
	"github.com/stretchr/testify/assert"
)

const mockURL = "https://abc-back.yandex-team.ru/api/v4/duty/shifts/?date_from=2020-08-25&date_to=2020-08-26&service=3910"

func TestGetShifts(t *testing.T) {
	// For asserts
	trueValue := true
	falseValue := false

	testCases := []struct {
		name           string
		handler        http.HandlerFunc
		expectedResult []abc.Shift
		expectedError  error
	}{
		{
			"get abc shifts: response ok",
			http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				_, _ = w.Write([]byte(`
				{
					"results": [
						{
							"id":359182,
							"person": {
								"login":"uplink",
								"uid":"1120000000012881"
							},
							"schedule": {
								"id":1330,
								"name":"boopy"
							},
							"replaces": [
								{
									"id":359194,
									"person": {
										"login":"megaeee",
										"uid":"1120000000043349"
									},
									"schedule": {
										"id":1330,
										"name":"boopy"
									},
									"is_approved":true,
									"start":"2020-08-24",
									"end":"2020-08-25",
									"start_datetime":"2020-08-24T00:00:00+03:00",
									"end_datetime":"2020-08-25T00:00:00+03:00"
								}
							],
							"is_approved":true,
							"start":"2020-08-23",
							"end":"2020-08-27",
							"start_datetime":"2020-08-23T00:00:00+03:00",
							"end_datetime":"2020-08-27T00:00:00+03:00"
						},
						{
							"id":359183,
                            "from_watcher":true,
							"person": {
								"login":"uplink",
								"uid":"1120000000012881"
							},
							"schedule": {
								"id":1330,
								"name":"boopy"
							},
							"is_approved":true,
							"is_primary":true,
							"start":"2020-08-23",
							"end":"2020-08-27",
							"start_datetime":"2020-08-23T00:00:00+03:00",
							"end_datetime":"2020-08-27T00:00:00+03:00"
						},
						{
							"id":359184,
							"from_watcher":true,
							"person": {
								"login":"megaeee",
								"uid":"1120000000043349"
							},
							"schedule": {
								"id":1330,
								"name":"boopy"
							},
							"is_approved":true,
							"is_primary":false,
							"start":"2020-08-23",
							"end":"2020-08-27",
							"start_datetime":"2020-08-23T00:00:00+03:00",
							"end_datetime":"2020-08-27T00:00:00+03:00"
						}
					]
				}`))
			}),
			func() []abc.Shift {
				return []abc.Shift{
					{
						ID:          359182,
						FromWatcher: false,
						Person: abc.Person{
							Login: "uplink",
							UID:   "1120000000012881",
						},
						Schedule: abc.Schedule{
							ID:   0x532,
							Name: "boopy",
						},
						IsApproved: true,
						Start:      "2020-08-23T00:00:00+03:00",
						End:        "2020-08-27T00:00:00+03:00",
						Replaces: []abc.Shift{
							{
								ID: 359194,
								Person: abc.Person{
									Login: "megaeee",
									UID:   "1120000000043349",
								},
								Schedule: abc.Schedule{
									ID:   0x532,
									Name: "boopy",
								},
								IsApproved: true,
								Start:      "2020-08-24T00:00:00+03:00",
								End:        "2020-08-25T00:00:00+03:00",
								Replaces:   []abc.Shift(nil),
							},
						},
					},
					{
						ID:          359183,
						IsPrimary:   &trueValue,
						FromWatcher: true,
						Person: abc.Person{
							Login: "uplink",
							UID:   "1120000000012881",
						},
						Schedule: abc.Schedule{
							ID:   0x532,
							Name: "boopy",
						},
						IsApproved: true,
						Start:      "2020-08-23T00:00:00+03:00",
						End:        "2020-08-27T00:00:00+03:00",
						Replaces:   []abc.Shift(nil),
					},
					{
						ID:          359184,
						IsPrimary:   &falseValue,
						FromWatcher: true,
						Person: abc.Person{
							Login: "megaeee",
							UID:   "1120000000043349",
						},
						Schedule: abc.Schedule{
							ID:   0x532,
							Name: "boopy",
						},
						IsApproved: true,
						Start:      "2020-08-23T00:00:00+03:00",
						End:        "2020-08-27T00:00:00+03:00",
						Replaces:   []abc.Shift(nil),
					},
				}
			}(),
			nil,
		},
		// {
		// 	"get abc shifts: malformed reponse",
		// 	http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		// 		_, _ = w.Write([]byte(``))
		// 	}),
		// 	[]abc.Shift{{}},
		// 	errors.New("malformed response: unexpected end of JSON input"),
		// },
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			ts := httptest.NewServer(tc.handler)
			cli, _ := NewClient(ts.URL, "api/v4/duty/shifts")
			shs, err := cli.GetShifts(time.Now(), time.Now(), 0, 0)
			if tc.expectedError == nil {
				assert.NoError(t, err)
				assert.Equal(t, tc.expectedResult, shs)
			} else {
				assert.EqualError(t, tc.expectedError, err.Error())
			}
			ts.Close()
		})
	}
}
