./offline-mc-composite.py \
 --mc-serpsets-tar ~/arcadia/quality/mstand_metrics/users/snadira/offline_metrics/kochurovvs.tar.gz \
 --batch ~/arcadia/quality/mstand_metrics/users/snadira/offline_metrics/all_metrics_batch.json \
 --source ~/arcadia/quality/mstand_metrics/users/snadira/offline_metrics \
 --serp-attrs headers \
 --component-attrs long.videoHdFlag,webadd,RELEVANCE \
 --judgements video_quality_google,ads_at_hosts2,RELEVANCE \
 --no-use-cache \
 --output-file "out/result_pool.json"
