all: prepare_cf3 cf3 convert_for_sorting

prepare_cf3: prepare_cf3.cc Makefile
	g++ -Wall -O4 -o prepare_cf3 prepare_cf3.cc

cf3: cf3.cc Makefile
	g++ -Wall -O4 -march=native -mtune=native -mfpmath=sse -o cf3 cf3.cc

convert_for_sorting: convert_for_sorting.c Makefile
	gcc -Wall -O4 -march=native -mtune=native -mfpmath=sse -o convert_for_sorting convert_for_sorting.c

clean: 
	rm -f convert_for_sorting cf3 prepare_cf3
