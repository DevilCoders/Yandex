package spinnaker

import (
	"encoding/json"
	"path"
	"tool/cli"
	"tool/exec"
	"tool/file"
)

#pipelines_path: "pipelines"
#default_project: "default"

import_task: {
	#application: string
	#pipeline: string
	#dir: string | *#pipelines_path
	#log: bool | *true

	pipeline: exec.Run & {
		cmd: [
			"spin", "-q", "pipeline", "get",
			if #application != "" {
				"--application=\(#application)"
			},
			if #pipeline != "" {
				"--name=\(#pipeline)"
			}
		]
		stdout: string
		success: true
	}

	#filename: path.Join([#dir, #default_project, #application, "\(#pipeline).json"])
	#dirname: path.Dir(#filename)

	#pipeline_json: json.Marshal({
		for k, v in json.Unmarshal(pipeline.stdout) if k != "updateTs" {
			(k): v
		}
	})

	valid: json.Validate(#pipeline_json, #Pipeline)

	mkdir: file.MkdirAll & {
		path: #dirname
	}

	save: file.Create & {
		filename: #filename
		contents: json.Indent(#pipeline_json, "", " ")
		$dep: mkdir.$done
	}

	if #log {
		log: cli.Print & {
			text: "Succesfully imported pipeline \(#pipeline) from application \(#application) to \(save.filename)"
			$dep: save.$done
		}
		$done: log.$done
	}

	if !#log {
		$done: save.$done
	}
}

export_task: {
	#filepath: string
	#dryRun: bool

	if #dryRun {
		read: file.Read & {
			filename: #filepath
		}

		#json: json.Unmarshal(read.contents)

		tmpdir: file.MkdirTemp & {
			pattern: "spinnaker-cue-export-dryrun"
		}

		import: import_task & {
			#application: #json.application
			#pipeline: #json.name
			#dir: tmpdir.path
			#log: false
		}

		diff: exec.Run & {
			cmd: [
				"sh",
				"-c",
				"diff -u \"\(import.#filename)\" \"\(#filepath)\" || true"
			]
			$dep: import.$done
		}

		cleanup: file.RemoveAll & {
			path: tmpdir.path
			$dep: diff.$done
		}
	}
	if !#dryRun {
		save: exec.Run & {
			cmd: "spin -q pipeline save -f \(#filepath)"
			success: true
		}

		log: cli.Print & {
			text: "Successfully exported pipeline \(#filepath) to spinnaker"
			$dep: save.$done
		}
	}
}
