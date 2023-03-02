CCFLAGS ?= -Wall -Ofast -DNDEBUG -std=c++20 -lpthread -lopenblas -I/usr/local/include -I/opt/homebrew/Cellar/blaze/3.8.2/include -L/opt/homebrew/opt/openblas/lib -I/opt/homebrew/opt/openblas/include
SKFLAGS ?= -Wall -Ofast -DNDEBUG -std=c++20 -I/usr/local/include
all_targets = skiplist

.PHONY: sqpoll cache

all: $(all_targets)

clean:
	rm -f $(all_targets)

skiplist:
	g++ skiplist.cc -o skiplist  ${SKFLAGS}

cache:
	g++ cache.cc -o cache  ${CCFLAGS}
