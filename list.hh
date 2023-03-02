#pragma once
  
template <typename T>
struct sll_node
{
    T data;
    T* prev;
    T* next;
  
    // Default constructor
    sll_node(T d)
    {
        data = 0;
        next = NULL;
    }

    void init(T& d, T* p, T* n)
    {
        data = d;
        prev = p;
        next = n;
    }
};



// Linked list class to
// implement a linked list.
class sll
{
    vector<sll_node<T>> vec_;
    
    Node* head;
    Node* tail;
    Node* unused;

public:
    sll() { head = NULL; }
  
    void insert_front(T* d)
    {
        Node* cur;

        if (unused) {
            Node* next = unused->next;
            cur = unused;
            unused = next;
        } else {
            vec_.push_back(sll_node<T>());
            cur = vec_.back_ptr();
        }

        cur->init(d, nullptr, head);
        head->prev = cur;

        head = cur;
    }
  
    void delete_node(sll_node<T>* node)
    {
        if (!node)
            return;

        if (node->prev)
            node->prev->next = node->next;
        
        if (node->next)
            node->next->prev = node->prev;

        node->prev = nullptr;
        node->next = unused;
        unused = node;
    }

    void move_to_front(sll_node<T>* node)
    {
        node->next = head;
        head = node;
    }

    void pop_back()
    {
        if (!tail)
            return;

        if (tail->prev)
            tail->prev->next = nullptr;

        tail->prev = nullptr;
        tail->next = unused;
        unused = tail;
    }
};
