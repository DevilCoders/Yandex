package spinnaker

import (
	"path"
	"tool/file"
)

//For some strange reason it does not work with a dir created by file.MkdirTemp
//So we can not reuse it in export_tool.cue
import_application: {
	app_name:      string
	pipelines_dir: path.Join([#applications_path, app_name, "pipelines"])
	mkdir:         file.Mkdir & {
		createParents: true
		permissions:   0o775
		path:          pipelines_dir
	}

	get: get_application_with_pipelines_from_spinnaker & {
		name: app_name
	}
	converted: convert_application_for_files & {
		input: get.result
	}
	save: save_application_with_pipelines_to_files & {
		$after: mkdir
		input:  converted.result
		dir:    #applications_path
	}

}

// Import an application with all its pipelines
//
// Import an application with all its pipelines
// An application name must be specified by a tag
// Example:
// cue cmd -t application=bootstrap-base-role import-application
command: "import-application": {
	args: {
		application: string @tag(application)
	}
	import: import_application & {
		app_name: args.application
	}
}

// Import a project with all its applications and their pipelines
//
// Import a project with all its applications and their pipelines
// A project name must be specified by a tag.
// Example:
// cue cmd -t project=EngInfra import-project-applications
command: "import-project-applications": {
	args: {
		project_name: string @tag(project)
	}

	get: get_project_from_spinnaker & {
		project_name: args.project_name
	}
	project: project_file_format & {
		object: get.result
	}

	mkdir: file.Mkdir & {
		createParents: true
		permissions:   0o775
		path:          #projects_path
	}

	save: save_project_to_file & {
		$after: mkdir
		input:  project
	}

	applications: {
		for app in project.object.config.applications {
			(app): import_application & {
				app_name: app
				log:      true
			}
		}
	}
}

// Import all controlled applications with pipelines from spinnaker
//
// Import all controlled applications with pipelines from spinnaker
// In order to import a new application, that is not controlled yet,
// use import-application or import-project-applications
command: "import-all-applications": {
	applications: file.Glob & {
		glob: path.Join([#applications_path, "*"])
	}
	for _, filepath in applications.files {
		(filepath): {
			#app_name: path.Base(filepath)
			import:    import_application & {
				app_name: #app_name
				log:      true
			}
		}

	}
}
