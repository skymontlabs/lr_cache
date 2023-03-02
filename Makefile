CCFLAGS ?= -Wall -Ofast -DNDEBUG -std=c++20 -lpthread -lopenblas -I/usr/local/include -I/opt/homebrew/Cellar/blaze/3.8.2/include -L/opt/homebrew/opt/openblas/lib -I/opt/homebrew/opt/openblas/include
all_targets = cache

.PHONY: sqpoll cache

all: $(all_targets)

clean:
	rm -f $(all_targets)

cache:
	g++ skiplist.cc -o cache  ${CCFLAGS}
