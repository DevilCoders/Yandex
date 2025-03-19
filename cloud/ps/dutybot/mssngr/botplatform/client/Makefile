ifneq ($(F),)
    TEST_FILTERS=--test-filter $(F)
endif

test:
	ya make -t -A $(TEST_FILTERS)
	mypy . --strict
