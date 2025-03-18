set -e -x
PYTHONPATH=. venv/venv/bin/python ./yp_tools/make_recluster_request.py $1 
~/arcadia/search/tools/yp_alloc/yp_alloc $1.json $1.json 2>&1 | tee $1.out
