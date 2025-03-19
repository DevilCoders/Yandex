package spinnaker

import (
	"path"
	"encoding/json"
)

#projects_path:     "projects"
#applications_path: "applications"

#pipelines_subdir: "pipelines"

get_from_spinnaker: {
	cmd: string
	let Cmd = cmd
	get: {
		$id:     "tool/exec.Run"
		cmd:     Cmd
		stdout:  string
		success: true
	}
	result: json.Unmarshal(get.stdout)
}

get_project_from_spinnaker: get_from_spinnaker & {
	project_name: string
	cmd:          "spin -q project get \(project_name)"
}

get_application_from_spinnaker: get_from_spinnaker & {
	application_name: string
	cmd:              "spin -q application get \(application_name)"
}

get_application_pipelines_from_spinnaker: get_from_spinnaker & {
	application_name: string

	cmd: "spin -q pipelines list -a \(application_name)"
}

get_application_with_pipelines_from_spinnaker: {
	name: string

	get_app: get_application_from_spinnaker & {
		application_name: name
	}
	get_pipelines: get_application_pipelines_from_spinnaker & {
		application_name: name
	}
	result: application_with_pipilenes_spinnaker & {
		application: get_app.result
		pipelines:   get_pipelines.result
	}
}

save_project_to_file: {
	input:    project_file_format
	fullpath: path.Join([#projects_path, "\(input.object.name).json"])
	save: {
		$id:         "tool/file.Create"
		filename:    fullpath
		contents:    input.fileFormat
		permissions: 0o664
	}
}

//Save an application with its pipelines to a directory
//Expectes that all required directories and subdirectories for an application exist
save_application_with_pipelines_to_files: {
	input: application_with_pipilenes_file_format
	dir:   string

	pipelines_dir: path.Join([dir, input.application.object.name, "pipelines"])
	save: {
		$id:         "tool/file.Create"
		filename:    path.Join([dir, input.application.object.name, "application.json"])
		contents:    input.application.fileFormat
		permissions: 0o664
	}

	save_pipelines: {
		for i, pipeline in input.pipelines {
			"pipeline_\(i)": {
				$id:         "tool/file.Create"
				filename:    path.Join([pipelines_dir, "\(pipeline.object.name).json"])
				contents:    pipeline.fileFormat
				permissions: 0o664
			}
		}
	}

}
