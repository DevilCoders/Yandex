package spinnaker

import (
	"path"
	"tool/file"
)

args: {
	dryRun:   bool | *true @tag(dryrun,type=bool)
}

// Export all pipelines to spinnaker, if dryRun is specified - only show diff
//
// Export all pipelines to spinnaker, if dryRun is specified - only show diff
// Examples:
// cue cmd export-all
// cue cmd dryrun=false export-all
command: "export-all": {
	list: file.Glob & {
		glob: path.Join([#pipelines_path, #default_project, "*", "*.json"])
	}

	for _, filepath in list.files {
		(filepath): export_task & {
			#filepath: filepath
			#dryRun: args.dryRun
		}
	}
}
