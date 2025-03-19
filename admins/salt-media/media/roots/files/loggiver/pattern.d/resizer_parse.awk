{
 if ($0 ~ /generate thumbnail/) {match($0, /spent-time="([0-9]+)ms"/, gt); (gt_sum+=gt[1])} else
 if ($0 ~ /read thumbnail/) {match($0, /spent-time="([0-9]+)ms"/, rt); (rt_sum+=rt[1])} else
 if ($0 ~ /read orig from cache/) {match($0, /spent-time="([0-9]+)ms"/, rofc); (rofc_sum+=rofc[1])} else
 if ($0 ~ /download file/) {match($0, /spent-time="([0-9]+)ms"/, df); (df_sum+=df[1])} else
 if ($0 ~ /write orig into cache/) {match($0, /spent-time="([0-9]+)ms"/, woic); (woic_sum+=woic[1])} else
 if ($0 ~ /write thumbnail into cache/) {match($0, /spent-time="([0-9]+)ms"/, wtic); (wtic_sum+=wtic[1])} else
 if ($0 ~ /write response/) {match($0, /spent-time="([0-9]+)ms"/, wr); (wr_sum+=wr[1])}

}
END { 
print "generate_thumbnail",sprintf("%.3f",gt_sum/1000000)"\n""read_thumbnail",sprintf("%.3f",rt_sum/1000000)"\n""read_orig_from_cache ", sprintf("%.3f", rofc_sum/1000000)"\n""download_file ", sprintf("%.3f", df_sum/1000000)"\n""write_orig_into_cache ", sprintf("%.3f", woic_sum/1000000)"\n""write_thumbnail_into_cache ", sprintf("%.3f", wtic_sum/1000000)"\n""write_response ", sprintf("%.3f", wr_sum/1000000)
}
