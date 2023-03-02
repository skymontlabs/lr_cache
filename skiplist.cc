#include "skiplist.hh"

int main()
{
    int arr[] = { 3, 6, 9, 2, 3, 11, 1, 4 }, i;
    skiplist<int, int> list;
    vector<snode<int, int>*> nodes(32);

    for (i = 0; i < sizeof(arr) / sizeof(arr[0]); i++) {
        printf("inserted: %d\n", arr[i]);
        nodes.push_back(list.emplace(arr[i], i));
    }

    printf("%d %d\n",nodes[0]->key,nodes[0]->value);
    
    snode<K, V>* nod = skiplist_move(13, nodes[0]);

    list.dump();

    return 0;
}
