CXX=g++
RM=rm -f
CPPFLAGS=-g -pedantic -Wall -O2

SRCS=write_random_bytes.cpp
OBJS=$(subst .cpp,.o,$(SRCS))

all: write

write: $(SRCS)
	$(CXX) $(CPPFLAGS) -o write_random_bytes $(SRCS)

clean:
	$(RM) write_random_bytes
