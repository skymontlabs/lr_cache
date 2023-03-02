#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include "util/vector.hh"

#define SKIPLIST_MAX_LEVEL 6

template <typename K, typename V>
struct snode
{
    int key;
    int value;
    snode* forward[SKIPLIST_MAX_LEVEL + 1];
};

static int rand_level()
{
    int level = 1;
    while (rand() < RAND_MAX / 2 && level < SKIPLIST_MAX_LEVEL)
        level++;
    return level;
}

template <typename K, typename V>
static void skiplist_node_free(snode<K, V>* x)
{
    if (x) {
        free(x->forward);
        free(x);
    }
}

template <typename K, typename V>
struct skiplist
{
    int max_level;
    snode<K, V> *hdr;
    vector<snode<K, V>> allocator_;

    skiplist():
    max_level(1),
    allocator_(256)
    {
        snode<K, V>* header = allocator_.alloc_back();

        header->key = INT_MAX;
        for (int i = 0; i <= SKIPLIST_MAX_LEVEL; ++i)
            header->forward[i] = header;

        hdr = header;
    }

    ~skiplist()
    {
        /*
        snode<K, V>* current_node = this->hdr->forward[1];
        while(current_node != this->hdr) {
            snode<K, V>* next_node = current_node->forward[1];
            //free(current_node->forward);
            free(current_node);
            current_node = next_node;
        }
        //free(current_node->forward);
        free(current_node);
        free(list);*/
    }


    void emplace(K key, V value)
    {
        snode<K, V>* update[SKIPLIST_MAX_LEVEL + 1];
        snode<K, V>* x = this->hdr;
        int i, level;

        //puts("declare");
        //printf("start ptr: %x\n", x);

        for (i = max_level; i >= 1; --i) {
            printf("level: %d key: %d\n", i, x->forward[i]->key);
            while (x->forward[i]->key <= key)
                x = x->forward[i];
            update[i] = x;
        }
        x = x->forward[1];

        //puts("searched");

        // we want to be like a multimap
        level = rand_level();
        if (level > max_level) {
            for (i = max_level + 1; i <= level; ++i)
                update[i] = this->hdr;
            this->max_level = level;
        }

        //puts ("checkleveled");

        x = allocator_.alloc_back();
        x->key = key;
        x->value = value;

        for (i = 1; i <= level; i++) {
            x->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = x;
        }
    }

    snode<K, V>* skiplist_search(K& key)
    {
        snode<K, V>* x = this->hdr;
        for (int i = this->max_level; i >= 1; i--) {
            while (x->forward[i]->key < key)
                x = x->forward[i];
        }
        
        if (x->forward[1]->key == key)
            return x->forward[1];

        return NULL;
    }

    snode<K, V>* skiplist_move(K& key, snode<K, V>* x)
    {
        for (int i = this->max_level; i >= 1; i--) {
            if (x->forward[i]) {
                while (x->forward[i]->key < key)
                    x = x->forward[i];
            }
        }
        
        if (x->forward[1]->key == key) {
            // that means we 
            return x->forward[1];
        }

        return NULL;
    }


    int skiplist_delete(K& key)
    {
        int i;
        snode<K, V>* update[SKIPLIST_MAX_LEVEL + 1];
        snode<K, V>* x = this->hdr;
        for (i = this->max_level; i >= 1; i--) {
            while (x->forward[i]->key < key)
                x = x->forward[i];
            update[i] = x;
        }

        x = x->forward[1];
        if (x->key == key) {
            for (i = 1; i <= this->max_level; ++i) {
                if (update[i]->forward[i] != x)
                    break;
                update[i]->forward[i] = x->forward[i];
            }
            skiplist_node_free(x);

            while (this->max_level > 1 && this->hdr->forward[this->max_level] == this->hdr)
                this->max_level--;
            return 0;
        }
        return 1;
    }

    void dump()
    {
        snode<K, V>* x = this->hdr;
        while (x && x->forward[1] != this->hdr) {
            printf("%d[%d]->", x->forward[1]->key, x->forward[1]->value);
            x = x->forward[1];
        }
        printf("NIL\n");
    }
};
