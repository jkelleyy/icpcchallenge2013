#ifndef BISTROMATHICS_SIMPLE_STACK_H
#define BISTROMATHICS_SIMPLE_STACK_H 1

#include <cstddef>

//a simple stack that shares data
template<typename T>
class SimpleStack{
private:
    struct Node{
        Node(const T& v, Node *n) : refCount(0),val(v), next(n){
            if(next)
                next->refCount++;
        }
        ~Node(){
            if(next){
                next->refCount--;
                if(next->refCount==0){
                    delete next;
                }
            }
        }
        int refCount;
        T val;
        Node *next;
    };
    Node *root;
public:
    SimpleStack() : root(NULL) {}
    SimpleStack(const SimpleStack& other) : root(other.root){
        if(root)
            root->refCount++;
    }
    ~SimpleStack(){
        if(root){
            root->refCount--;
            if(root->refCount==0)
                delete root;
        }
    }
    SimpleStack& operator=(const SimpleStack& other){
        root = other.root;
        if(root)
            root->refCount++;
        return *this;
    }
    bool empty(){
        return root == NULL;
    }
    void pop(){
        Node *temp = root;
        root = root->next;
        if(root)
            root->refCount++;
        temp->refCount--;
        if(temp->refCount==0)
            delete temp;
    }
    T peek(){
        return root->val;
    }
    void push(const T& val){
        if(root)
            root->refCount--;
        root = new Node(val,root);
        root->refCount++;
    }
};

#endif
