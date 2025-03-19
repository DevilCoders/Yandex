.PHONY: run

run:
	docker build -t mdb_secretsdb_test .
	docker run --rm -p 5432:5432 -t mdb_secretsdb_test
