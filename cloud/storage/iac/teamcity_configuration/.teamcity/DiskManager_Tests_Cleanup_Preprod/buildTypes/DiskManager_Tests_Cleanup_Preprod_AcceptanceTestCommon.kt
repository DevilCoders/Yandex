package DiskManager_Tests_Cleanup_Preprod.buildTypes

import jetbrains.buildServer.configs.kotlin.v2019_2.*
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.ScriptBuildStep
import jetbrains.buildServer.configs.kotlin.v2019_2.buildSteps.script

object DiskManager_Tests_Cleanup_Preprod_AcceptanceTestCommon : BuildType({
    templates(_Self.buildTypes.Nbs_Cleanup_YcCleanup)
    name = "Acceptance test (common)"
    description = "Remove stale entities from acceptance-test binary (common)"
    paused = true

    params {
        param("env.DISK_REGEX", """^"acceptance-test-disk-[0-9]+"${'$'}""")
        param("env.INSTANCE_TTL_DAYS", "")
        param("env.SNAPSHOT_TTL_DAYS", "2")
        param("env.IMAGE_REGEX", """^"acceptance-test-image-[0-9]+"${'$'}""")
        param("env.SNAPSHOT_REGEX", """^"acceptance-test-snapshot-[0-9]+"${'$'}""")
    }

    steps {
        script {
            name = "Remove stale instances"
            id = "RUNNER_6805"
            enabled = false
            scriptContent = """
                #!/usr/bin/env bash
                
                set -e
                
                if [ -z "${'$'}CLUSTER" ]
                then
                    echo "Enter profile id" >&2
                    exit 1
                fi
                
                profile_id=${'$'}CLUSTER
                echo "Ycp profile: \"${'$'}profile_id\""
                
                if [ -z "${'$'}INSTANCE_REGEX" ]
                then
                    echo "Enter regex for instances to delete" >&2
                    exit 1
                fi
                
                instance_name_regex=${'$'}INSTANCE_REGEX
                echo "Instance name regex: \"${'$'}instance_name_regex\""
                
                if [ -z "${'$'}INSTANCE_TTL_DAYS" ]
                then
                    echo "Enter ttl days for instances to delete" >&2
                    exit 1
                fi
                
                echo "Instance ttl days: \"${'$'}INSTANCE_TTL_DAYS\""
                expected_time_diff=${'$'}(( ${'$'}INSTANCE_TTL_DAYS * 24 * 60 * 60 ))
                echo "Expected ttl seconds: \"${'$'}expected_time_diff\""
                
                IFS=${'$'}'\n'
                
                now_since_epoch=${'$'}(date +"%s")
                echo "Number of seconds since the epoch: \"${'$'}now_since_epoch\""
                
                for line in ${'$'}(ycp compute instance list --profile ${'$'}profile_id --format json | jq '.[] | .id')
                do
                    id=${'$'}(echo "${'$'}line" | sed 's/\"//g')
                    instance_stats=${'$'}(ycp compute instance get ${'$'}id --profile ${'$'}profile_id --format json)
                    
                    if [ "${'$'}(echo "${'$'}instance_stats" | jq 'has("name")')" != "true" ]
                    then
                        continue
                    fi
                    
                    name=${'$'}(echo "${'$'}instance_stats" | jq '.name')
                    
                    if [ -z ${'$'}(echo "${'$'}name" | grep -E ${'$'}instance_name_regex) ]
                    then
                        continue
                    fi
                    
                    created_at=${'$'}(echo "${'$'}instance_stats" | jq ".created_at" | sed 's/["Z]//g')
                    created_since_epoch=${'$'}(date --date ${'$'}created_at +"%s")
                    time_diff=${'$'}(( ${'$'}now_since_epoch - ${'$'}created_since_epoch ))
                    
                    echo "=== Found instance: ==="
                    echo "Id: ${'$'}id"
                    echo "Name: ${'$'}name"
                    echo "Created_at: ${'$'}created_at"
                    echo "Created_since_epoch: ${'$'}created_since_epoch"
                    echo "Time_diff: ${'$'}time_diff"
                    
                    if [ ${'$'}time_diff -ge ${'$'}expected_time_diff ]
                    then
                        echo -n "INSTANCE IS STALE"
                        ycp compute instance delete ${'$'}id --profile ${'$'}profile_id
                        echo ": removal was performed"
                    else
                        echo "INSTANCE IS STILL FRESH: removal was not performed"
                    fi
                    
                    echo "========================"
                done
                
                echo "The script has successfully finished"
            """.trimIndent()
            dockerImagePlatform = ScriptBuildStep.ImagePlatform.Linux
            dockerPull = true
            dockerImage = "%yc-nbs-ci-tools.docker.image%"
            dockerRunParameters = """--rm --add-host="local-lb.cloud-lab.yandex.net:2a02:6b8:bf00:1300:9a03:9bff:feaa:b659" --network host"""
        }
        script {
            name = "Remove stale disks"
            id = "RUNNER_12978"
            scriptContent = """
                #!/usr/bin/env bash
                
                set -e
                
                if [ -z "${'$'}CLUSTER" ]
                then
                    echo "Enter profile id" >&2
                    exit 1
                fi
                
                profile_id=${'$'}CLUSTER
                echo "Ycp profile: \"${'$'}profile_id\""
                
                if [ -z "${'$'}DISK_REGEX" ]
                then
                    echo "Enter regex for disks to delete" >&2
                    exit 1
                fi
                
                disk_name_regex=${'$'}DISK_REGEX
                echo "Disk name regex: \"${'$'}disk_name_regex\""
                
                if [ -z "${'$'}DISK_TTL_DAYS" ]
                then
                    echo "Enter ttl days for disks to delete" >&2
                    exit 1
                fi
                
                echo "Disk ttl days: \"${'$'}DISK_TTL_DAYS\""
                expected_time_diff=${'$'}(( ${'$'}DISK_TTL_DAYS * 24 * 60 * 60 ))
                echo "Expected ttl seconds: \"${'$'}expected_time_diff\""
                
                IFS=${'$'}'\n'
                
                now_since_epoch=${'$'}(date +"%s")
                echo "Number of seconds since the epoch: \"${'$'}now_since_epoch\""
                
                for line in ${'$'}(ycp compute disk list --profile ${'$'}profile_id --format json | jq '.[] | .id')
                do
                    id=${'$'}(echo "${'$'}line" | sed 's/\"//g')
                    disk_stats=${'$'}(ycp compute disk get ${'$'}id --profile ${'$'}profile_id --format json)
                    
                    if [ "${'$'}(echo "${'$'}disk_stats" | jq 'has("name")')" != "true" ]
                    then
                        continue
                    fi
                    
                    name=${'$'}(echo "${'$'}disk_stats" | jq '.name')
                    
                    if [ -z ${'$'}(echo "${'$'}name" | grep -E ${'$'}disk_name_regex) ]
                    then
                        continue
                    fi
                    
                    created_at=${'$'}(echo "${'$'}disk_stats" | jq '.created_at' | sed 's/["Z]//g')
                    created_since_epoch=${'$'}(date --date ${'$'}created_at +"%s")
                    time_diff=${'$'}(( ${'$'}now_since_epoch - ${'$'}created_since_epoch ))
                    
                    echo "=== Found disk: ==="
                    echo "Id: ${'$'}id"
                    echo "Name: ${'$'}name"
                    echo "Created_at: ${'$'}created_at"
                    echo "Created_since_epoch: ${'$'}created_since_epoch"
                    echo "Time_diff: ${'$'}time_diff"
                    
                    if [ ${'$'}time_diff -ge ${'$'}expected_time_diff ]
                    then
                        echo -n "DISK IS STALE"
                        if [ "${'$'}(echo "${'$'}disk_stats" | jq 'has("instance_ids")')" == "true" ]
                        then
                            echo ": removal was not performed, because disk is attached"
                        else
                            ycp compute disk delete ${'$'}id --profile ${'$'}profile_id
                            echo ": removal was performed"
                        fi
                    else
                        echo "DISK IS STILL FRESH: removal was not performed"
                    fi
                    
                    echo "==================="
                done
                
                echo "The script has successfully finished"
            """.trimIndent()
            dockerImagePlatform = ScriptBuildStep.ImagePlatform.Linux
            dockerPull = true
            dockerImage = "%yc-nbs-ci-tools.docker.image%"
            dockerRunParameters = """--rm --add-host="local-lb.cloud-lab.yandex.net:2a02:6b8:bf00:1300:9a03:9bff:feaa:b659" --network host"""
        }
        script {
            name = "Remove stale images"
            id = "RUNNER_13013"
            scriptContent = """
                #!/usr/bin/env bash
                
                set -e
                
                if [ -z "${'$'}CLUSTER" ]
                then
                    echo "Enter profile id" >&2
                    exit 1
                fi
                
                profile_id=${'$'}CLUSTER
                echo "Ycp profile: \"${'$'}profile_id\""
                
                if [ -z "${'$'}IMAGE_REGEX" ]
                then
                    echo "Enter regex for images to delete" >&2
                    exit 1
                fi
                
                image_name_regex=${'$'}IMAGE_REGEX
                echo "Image name regex: \"${'$'}image_name_regex\""
                
                if [ -z "${'$'}IMAGE_TTL_DAYS" ]
                then
                    echo "Enter ttl days for images to delete" >&2
                    exit 1
                fi
                
                echo "Image ttl days: \"${'$'}IMAGE_TTL_DAYS\""
                expected_time_diff=${'$'}(( ${'$'}IMAGE_TTL_DAYS * 24 * 60 * 60 ))
                echo "Expected ttl seconds: \"${'$'}expected_time_diff\""
                
                IFS=${'$'}'\n'
                
                now_since_epoch=${'$'}(date +"%s")
                echo "Number of seconds since the epoch: \"${'$'}now_since_epoch\""
                
                for line in ${'$'}(ycp compute image list --profile ${'$'}profile_id --format json | jq '.[] | .id')
                do
                    id=${'$'}(echo "${'$'}line" | sed "s/\"//g")
                    image_stats=${'$'}(ycp compute image get ${'$'}id --profile ${'$'}profile_id --format json)
                    
                    if [ "${'$'}(echo "${'$'}image_stats" | jq 'has("name")')" != "true" ]
                    then
                        continue
                    fi
                    
                    name=${'$'}(echo "${'$'}image_stats" | jq '.name')
                    
                    if [ -z ${'$'}(echo "${'$'}name" | grep -E ${'$'}image_name_regex) ]
                    then
                        continue
                    fi
                    
                    created_at=${'$'}(echo "${'$'}image_stats" | jq '.created_at' | sed 's/["Z]//g')
                    created_since_epoch=${'$'}(date --date ${'$'}created_at +"%s")
                    time_diff=${'$'}(( ${'$'}now_since_epoch - ${'$'}created_since_epoch ))
                    
                    echo "=== Found image: ==="
                    echo "Id: ${'$'}id"
                    echo "Name: ${'$'}name"
                    echo "Created_at: ${'$'}created_at"
                    echo "Created_since_epoch: ${'$'}created_since_epoch"
                    echo "Time_diff: ${'$'}time_diff"
                    
                    if [ ${'$'}time_diff -ge ${'$'}expected_time_diff ]
                    then
                        echo -n "IMAGE IS STALE"
                        ycp compute image delete ${'$'}id --profile ${'$'}profile_id
                        echo ": removal was performed"
                    else
                        echo "IMAGE IS STILL FRESH: removal was not performed"
                    fi
                    
                    echo "===================="
                done
                
                echo "The script has successfully finished"
            """.trimIndent()
            dockerImagePlatform = ScriptBuildStep.ImagePlatform.Linux
            dockerPull = true
            dockerImage = "%yc-nbs-ci-tools.docker.image%"
            dockerRunParameters = """--rm --add-host="local-lb.cloud-lab.yandex.net:2a02:6b8:bf00:1300:9a03:9bff:feaa:b659" --network host"""
        }
        script {
            name = "Remove stale snapshots"
            id = "RUNNER_13101"
            scriptContent = """
                #!/usr/bin/env bash
                
                set -e
                
                if [ -z "${'$'}CLUSTER" ]
                then
                    echo "Enter profile id" >&2
                    exit 1
                fi
                
                profile_id=${'$'}CLUSTER
                echo "Ycp profile: \"${'$'}profile_id\""
                
                if [ -z "${'$'}SNAPSHOT_REGEX" ]
                then
                    echo "Enter regex for snapshots to delete" >&2
                    exit 1
                fi
                
                snapshot_name_regex=${'$'}SNAPSHOT_REGEX
                echo "Snapshot name regex: \"${'$'}snapshot_name_regex\""
                
                if [ -z "${'$'}SNAPSHOT_TTL_DAYS" ]
                then
                    echo "Enter ttl days for snapshots to delete" >&2
                    exit 1
                fi
                
                echo "Snapshot ttl days: \"${'$'}SNAPSHOT_TTL_DAYS\""
                expected_time_diff=${'$'}(( ${'$'}SNAPSHOT_TTL_DAYS * 24 * 60 * 60 ))
                echo "Expected ttl seconds: \"${'$'}expected_time_diff\""
                
                IFS=${'$'}'\n'
                
                now_since_epoch=${'$'}(date +"%s")
                echo "Number of seconds since the epoch: \"${'$'}now_since_epoch\""
                
                for line in ${'$'}(ycp compute snapshot list --profile ${'$'}profile_id --format json | jq '.[] | .id')
                do
                    id=${'$'}(echo "${'$'}line" | sed "s/\"//g") 
                    snapshot_stats=${'$'}(ycp compute snapshot get ${'$'}id --profile ${'$'}profile_id --format json)
                    
                    if [ "${'$'}(echo "${'$'}snapshot_stats" | jq 'has("name")')" != "true" ]
                    then
                        continue
                    fi
                    
                    name=${'$'}(echo "${'$'}snapshot_stats" | jq '.name')
                    
                    if [ -z ${'$'}(echo "${'$'}name" | grep -E ${'$'}snapshot_name_regex) ]
                    then
                        continue
                    fi
                    
                    created_at=${'$'}(echo "${'$'}snapshot_stats" | jq '.created_at' | sed 's/["Z]//g')
                    created_since_epoch=${'$'}(date --date ${'$'}created_at +"%s")
                    time_diff=${'$'}(( ${'$'}now_since_epoch - ${'$'}created_since_epoch ))
                    
                    echo "=== Found snapshot: ==="
                    echo "Id: ${'$'}id"
                    echo "Name: ${'$'}name"
                    echo "Created_at: ${'$'}created_at"
                    echo "Created_since_epoch: ${'$'}created_since_epoch"
                    echo "Time_diff: ${'$'}time_diff"
                    
                    if [ ${'$'}time_diff -ge ${'$'}expected_time_diff ]
                    then
                        echo -n "SNAPSHOT IS STALE"
                        ycp compute snapshot delete ${'$'}id --profile ${'$'}profile_id
                        echo ": removal was performed"
                    else
                        echo "SNAPSHOT IS STILL FRESH: removal was not performed"
                    fi
                    
                    echo "======================="
                done
                
                echo "The script has successfully finished"
            """.trimIndent()
            dockerImagePlatform = ScriptBuildStep.ImagePlatform.Linux
            dockerPull = true
            dockerImage = "%yc-nbs-ci-tools.docker.image%"
            dockerRunParameters = """--rm --add-host="local-lb.cloud-lab.yandex.net:2a02:6b8:bf00:1300:9a03:9bff:feaa:b659" --network host"""
        }
        stepsOrder = arrayListOf("RUNNER_6805", "RUNNER_12978", "RUNNER_13013", "RUNNER_13101")
    }
})
