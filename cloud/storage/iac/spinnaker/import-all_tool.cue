package spinnaker

import (
	"encoding/json"
	"path"
	"tool/exec"
	"tool/file"
)

// Import all known applications pipelines from spinnaker
command: "import-all": {
	applications: file.Glob & {
		glob: path.Join([#pipelines_path, #default_project, "*"])
	}

	for _, filepath in applications.files {
		(filepath): {
			#application_name: path.Base(filepath)

			pipelines: exec.Run & {
				cmd: "spin -q pipeline list --application \(#application_name)"
				stdout: string
			}
			for _, pipeline in json.Unmarshal(pipelines.stdout) {
				(pipeline.name): import_task & {
					#application: #application_name
					#pipeline: pipeline.name
				}
			}
		}
	}
}
