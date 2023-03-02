#include "skiplist.hh"

int main()
{
    int arr[] = { 3, 6, 9, 2, 11, 1, 4 }, i;
    skiplist<int, int> list;

    for (i = 0; i < sizeof(arr) / sizeof(arr[0]); i++) {
        printf("inserted: %d\n", arr[i]);
        list.emplace(arr[i], i);
    }

    list.dump();

    return 0;
}