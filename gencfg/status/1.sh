/skynet/python/bin/python 0.py | sort -k1 > commit_status
svn log -l11000 ../db | grep ^r3 | awk '{print $1" "$5" "$6}' | tr -d 'r' | sort -k1 > commit_date
join commit_date commit_status > 123
python 1.py > 1.csv
