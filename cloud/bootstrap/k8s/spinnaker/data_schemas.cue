package spinnaker

import (
	"encoding/json"
)

//Allow arbitrary objects to be loaded from Spinnaker
application_with_pipilenes_spinnaker: {
	application: {...}
	pipelines: [...{...}]
}

//This schema defines format of an object saved in a file
object_file_format: {
	object: {...}
	no_system_fields: {
		{
			for k, v in object if k != "updateTs" && k != "lastModifiedBy" {
				(k): v
			}
		}
	}
	no_system_fields: #Project | #Application | GenericPipeline
	fileFormat:       string
	fileFormat:       json.Indent(json.Marshal(no_system_fields), "", "    ") + "\n"
}

project_file_format: object_file_format & {
	no_system_fields: #Project
}

application_with_pipilenes_file_format: application_with_pipilenes_spinnaker & {
	application: object_file_format & {
		no_system_fields: #Application
	}
	pipelines: [...object_file_format & {
		no_system_fields: GenericPipeline
	},
	]
}

convert_application_for_files: {
	input:  application_with_pipilenes_spinnaker
	result: application_with_pipilenes_file_format & {
		application: object_file_format & {
			object: input.application
		}
		pipelines: [ for pipeline in input.pipelines {
			object_file_format & {object: pipeline}
		}]
	}
}
