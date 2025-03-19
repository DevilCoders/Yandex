package http

import (
	"context"
	"net/http"
	"net/http/httptest"
	"testing"

	"github.com/stretchr/testify/assert"
)

type inputData struct {
	FolderID         string
	ServiceAccountID string
	RoleID           string
}

func TestCheckServiceAccountRole(t *testing.T) {
	testCases := []struct {
		name           string
		responses      []string
		input          inputData
		expectedResult bool
		isError        bool
	}{
		{
			name: "CheckServiceAccountRole found",
			responses: []string{
				`{
					"accessBindings": [
						{
							"subject": {
								"id": "accountID",
								"type": "serviceAccount"
							},
							"roleId": "role1"
						},
						{
							"subject": {
								"id": "accountID",
								"type": "serviceAccount"
							},
							"roleId": "role2"
						}
					],
					"nextPageToken": "nextToken"
				}`,
				`{
					"accessBindings": [
						{
							"subject": {
								"id": "accountID",
								"type": "serviceAccount"
							},
							"roleId": "role3"
						},
						{
							"subject": {
								"id": "accountID",
								"type": "serviceAccount"
							},
							"roleId": "role4"
						}
					],
					"nextPageToken": "nextToken"
				}`,
				`{}`,
			},
			input:          inputData{"myFolderID", "accountID", "role3"},
			expectedResult: true,
			isError:        false,
		},
		{
			name: "CheckServiceAccountRole not found",
			responses: []string{
				`{
					"accessBindings": [
						{
							"subject": {
								"id": "accountID",
								"type": "serviceAccount"
							},
							"roleId": "compute.images.user"
						},
						{
							"subject": {
								"id": "accountID",
								"type": "serviceAccount"
							},
							"roleId": "dataproc.agent"
						}
					],
					"nextPageToken": "nextToken"
				}`,
				`{}`,
			},
			input:          inputData{"myFolderID", "accountID1", "dataproc.agent"},
			expectedResult: false,
			isError:        false,
		},
		{
			name: "CheckServiceAccountRole malformed response",
			responses: []string{
				`{
					malformed
				}`,
			},
			input:          inputData{"myFolderID", "accountID1", "dataproc.agent"},
			expectedResult: false,
			isError:        true,
		},
	}

	for _, tc := range testCases {
		t.Run(tc.name, func(t *testing.T) {
			pageNum := 0
			handler := http.HandlerFunc(func() http.HandlerFunc {
				return func(w http.ResponseWriter, r *http.Request) {
					_, _ = w.Write([]byte(tc.responses[pageNum]))
					pageNum++
				}
			}())
			ts := httptest.NewServer(handler)
			client, _ := NewClient(ts.URL)
			ok, err := client.CheckServiceAccountRole(
				context.Background(), "token", tc.input.FolderID, tc.input.ServiceAccountID, tc.input.RoleID)

			if !tc.isError {
				assert.NoError(t, err)
				assert.Equal(t, tc.expectedResult, ok)
			} else {
				assert.Error(t, err)
			}
			ts.Close()
		})
	}
}
