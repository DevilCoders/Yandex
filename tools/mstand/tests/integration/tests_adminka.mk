INPUT_POOL = $(RUN_DIR)/input_pool.json

$(INPUT_POOL): | $(RUN_DIR)
	./adminka-get-pool.py \
		--output $(INPUT_POOL) \
		--queue-id 1 \
		--remove-filtered \
		28280

INPUT_POOL_FILTERED = $(RUN_DIR)/input_pool_filtered.json

$(INPUT_POOL_FILTERED): $(INPUT_POOL)
	./adminka-filter-pool.py \
		--input $(INPUT_POOL) \
		--output $(INPUT_POOL_FILTERED) \
		--min-duration 3