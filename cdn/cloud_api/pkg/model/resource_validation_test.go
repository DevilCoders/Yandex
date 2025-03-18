package model

import (
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/ptr"
)

func TestRewriteOptions(t *testing.T) {
	testCases := map[string]struct {
		rewriteOptions RewriteOptions
		err            bool
	}{
		"ok_1": {
			rewriteOptions: RewriteOptions{
				Regex:       ptr.String(`^(.+)$`),
				Replacement: ptr.String(`$1_source.json`),
			},
			err: false,
		},
		"ok_2": {
			rewriteOptions: RewriteOptions{
				Regex:       ptr.String(`^/decoded/storage.mds.y.net/(.*)$`),
				Replacement: ptr.String(`/${1}23456`),
			},
			err: false,
		},
		"ok_3": {
			rewriteOptions: RewriteOptions{
				Regex:       ptr.String(`/(vod_json_meta|kalproxy)/(.*)`),
				Replacement: ptr.String(`/$2`),
			},
			err: false,
		},
		"ok_4": {
			rewriteOptions: RewriteOptions{
				Regex:       ptr.String(`/([\w]+)/([\w]+)/(.*)`),
				Replacement: ptr.String(`/$3?category=$1`),
			},
			err: false,
		},
		"ok_5": {
			rewriteOptions: RewriteOptions{
				Regex:       ptr.String(`(\.ts$|Fragment)`),
				Replacement: ptr.String(`/chunks/?type=$1`),
			},
			err: false,
		},
		"err_1": {
			rewriteOptions: RewriteOptions{
				Regex:       ptr.String(`^/old_stub\/?.*_(\d+_\d+p|audio\d+_\w+)`),
				Replacement: ptr.String(`/get_stub/strm-stub/${s3_stub_name_prefix}_$1.mp4`),
			},
			err: true,
		},
		"err_2": {
			rewriteOptions: RewriteOptions{
				Regex:       ptr.String(`^`),
				Replacement: ptr.String(`$request_uri`),
			},
			err: true,
		},
		"err_3": {
			rewriteOptions: RewriteOptions{
				Regex:       ptr.String(`^`),
				Replacement: ptr.String(`$1`),
			},
			err: true,
		},
		"err_4": {
			rewriteOptions: RewriteOptions{
				Regex:       ptr.String(`^/(\d+)/(\w*)`),
				Replacement: ptr.String(`/asdfg/${3}4567`),
			},
			err: true,
		},
		"err_5": {
			rewriteOptions: RewriteOptions{
				Regex:       ptr.String(`^/(?P<prefix>.+?)/asdf/(?P<suffix>.+)`),
				Replacement: ptr.String(`/$prefix/$suffix`),
			},
			err: true,
		},
	}

	for testName, testCase := range testCases {
		testName, testCase := testName, testCase

		t.Run(testName, func(t *testing.T) {
			t.Parallel()

			err := testCase.rewriteOptions.Validate()
			if testCase.err {
				assert.Error(t, err)
			} else {
				assert.NoError(t, err)
			}
		})
	}
}
