#!/bin/bash

cat /srv/ftp/ott-drm/Amediateka_list.csv | while IFS=';' read -ra str ; do
    src_filename="${str[0]}";
    dst_filename="${str[1]}"
    dst_path="${str[2]}"
    subtitles="${str[3]/subtitle:/}"

    src_filebase="${src_filename%.*}"
    dst_filebase="${dst_filename%.*}"

    mkdir -p "/root/amedia/${src_filebase}"

    (
        cd  "/root/amedia/${src_filebase}"

        echo "processing input ${str[@]}"

        grep -qF "${dst_filebase};success" /srv/ftp/ott-drm/Amediateka_status.csv
        if [[ ${?} -ne 0 ]]; then
            if [[ ! -f "${src_filename}" ]]; then
                echo "search for ${src_filename} file on amedia"
                src_filepath=$(echo 'find /' | lftp -p 7822 -u {{ cred }} sftp://85.249.7.53 | grep "${src_filename}")
                echo "get ${src_filepath}" | lftp -p 7822 -u {{ cred }} sftp://85.249.7.53
            else
                echo "file ${src_filename} exists"
            fi

            if [[ ! -f "${dst_filename}" ]]; then
                ffmpeg -i "${src_filename}" -map 0:v -map 0:a -c:v copy -c:a:0 aac -ar 48000 -ab 256K -ac 2 \
                    -clev 1.414 -slev .5 -c:a:1 copy \
                    -metadata:s:v:0 language=rus -metadata:s:a:0 language=rus -metadata:s:a:1 language=eng \
                    -disposition:a:0 default -disposition:a:1 none -map_metadata -1 -map_chapters -1 "${dst_filename}"
                echo "ffmpeg done"
            else
                echo "file ${dst_filename} exists"
            fi

            mkdir -p "/srv/ftp/${dst_path}"
            mv "${dst_filename}" "/srv/ftp/${dst_path}/"

            if [[ "${subtitles}" == "true" ]] ; then
                subtitles_off="${str[4]/offset:/}"
                sub_filename="${src_filebase}.srt"

                echo "processing subtitles ${sub_filename} with offset ${subtitles_off}"

                if [[ ! -f "${sub_filename}" ]]; then
                    echo "search for ${sub_filename} file on amedia"
                    sub_filepath=$(echo 'find /' | lftp -p 7822 -u {{ cred }} sftp://85.249.7.53 | grep "${sub_filename}")
                    echo "get ${sub_filepath}" | lftp -p 7822 -u {{ cred }} sftp://85.249.7.53
                else
                    echo "file ${sub_filename} exists"
                fi

                if [[ ! -f "${dst_filebase}.srt" ]]; then
                    ffmpeg -itsoffset "${subtitles_off}" -i "${sub_filename}" "${dst_filebase}.srt"
                    echo "ffmpeg done"
                else
                    echo "file ${dst_filebase}.srt exists"
                fi

                mkdir -p /srv/ftp/${dst_path}
                mv ${dst_filebase}.srt /srv/ftp/${dst_path}/
            fi

            echo "${dst_filebase};success" >> /srv/ftp/ott-drm/Amediateka_status.csv
        else
            echo "${dst_filebase} already done"
        fi
    )

    rm -r "/root/amedia/${src_filebase}"
done
