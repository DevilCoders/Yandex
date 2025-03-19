package spinnaker

import (
	"tool/file"
	"tool/cli"
	"tool/exec"
	"path"
	//"strings"
	"encoding/json"
)

load_application_from_files: {
	app_name: string

	read: file.Read & {
		filename: path.Join([#applications_path, app_name, "application.json"])
		contents: string
	}

	list: file.Glob & {
		glob: path.Join([#applications_path, app_name, #pipelines_subdir, "*.json"])
	}
	pipelines_raw: {for filepath in list.files {
		(filepath): {file.Read & {
			filename: filepath
			contents: string
		}
		}
	}
	}

	application: application_with_pipilenes_file_format & {
		application: object: json.Unmarshal(read.contents)
		application: object: name: app_name
		pipelines: [ for filepath, pipeline in pipelines_raw {object_file_format & {object: json.Unmarshal(pipeline.contents)}}]
	}
}

// Export specified application to spinnaker
//
// Export specified application to spinnaker
// An application name must be provided as a tag
// Use dry-run=true tag to get the diff(default)
// Use dry-run=false tag to apply
// cue cmd -t application bootstrap export-application
// cue cmd -t application bootstrap dryrun=false export-application
command: "export-application": {
	args: {
		application: string       @tag(application)
		dryRun:      bool | *true @tag(dryrun,type=bool)
	}

	//Validate application on disk
	load: load_application_from_files & {
		app_name: args.application
	}
	if args.dryRun {
		tmpdir: file.MkdirTemp & {
			pattern: "spinnaker-cue-export-dryrun"
		}
		pipelines_dir: path.Join([tmpdir.path, args.application, "pipelines"])
		mkdir:         file.Mkdir & {
			createParents: true
			permissions:   0o775
			path:          pipelines_dir
		}

		get: get_application_with_pipelines_from_spinnaker & {
			name: args.application
		}
		converted: convert_application_for_files & {
			input: get.result
		}
		save: save_application_with_pipelines_to_files & {
			input: converted.result
			dir:   tmpdir.path
		}

		print: cli.Print & {
			$after: save
			text:   "Diff of \(tmpdir.path)/\(args.application) and \(#applications_path)/\(args.application):"
		}

		diff: exec.Run & {
			$after: print
			cmd: [
				"sh",
				"-c",
				"diff -u -r \(tmpdir.path)/\(args.application) \(#applications_path)/\(args.application) || true",
			]
		}
	}
	if !args.dryRun {
		application_dir: path.Join([#applications_path, args.application])
		pipelines_dir:   path.Join([application_dir, "pipelines"])

		save: exec.Run & {
			cmd:     "spin -q application save -f \(path.Join([application_dir, "application.json"]))"
			success: true
		}

		pipelines: file.Glob & {
			glob: path.Join([pipelines_dir, "*"])
		}

		for _, pipeline in pipelines.files {
			(pipeline): {
				save: exec.Run & {
					cmd:     "spin -q pipeline save -f \(pipeline)"
					success: true
				}
			}
		}

	}
}
