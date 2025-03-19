.PHONY: all

all:
	./usn_gen.sh
	./rhel_gen.sh
	find . -name '*.json' | xargs ./merge.py > res
	mv res errata.json
	rm -f errata.json.gz
	gzip -9 errata.json

clean:
	rm -rf ubuntu rhel res errata.json* .mypy_cache
