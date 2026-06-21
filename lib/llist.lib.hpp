#ifndef LINKED_LIST_LIB
#define LINKED_LIST_LIB
#include <iostream>
#include <string>
#include <vector>
struct Node{
public:
    std::string val;
    Node *next, *prev;
    Node(): val(), next(nullptr), prev(nullptr){};
    Node(const std::string&v): val(v), next(nullptr), prev(nullptr){};
};

class LList{
private:
    size_t sz;
    Node *head, *tail;
    void deleteAll(){
        Node* current = head;
        while (current) {
            Node* next = current->next;
            delete current;
            current = next;
        }
        sz = 0;
    }
    void deleteNode(Node *node){
        if (head == nullptr || node == nullptr) return;
        Node *prev = node->prev;
        Node *next = node->next;
        if (prev == nullptr && next == nullptr){
            head = tail = nullptr;
            delete node;
        }
        else if (prev == nullptr){
            head = node->next;
            head->prev = nullptr;
            delete node;
        }else if (next == nullptr){
            tail = node->prev;
            tail->next = nullptr;
            delete node;
        }else {
            next->prev = prev;
            prev->next = next;
            delete node;
        }
        sz--;
    }
public:
    // constructor
    LList(): sz(0), head(nullptr), tail(nullptr){};
    LList(const std::string&val){
        tail = head = new Node(val);
        sz = 1;
    }
    LList(const LList& other) : sz(0), head(nullptr), tail(nullptr) {
        Node* current = other.head;
        while (current) {
            push_back(current->val);
            current = current->next;
        }
    }
    LList& operator=(const LList& other){
        if (this != &other){
            deleteAll();
            head = tail = nullptr;
            sz = 0;
            Node* current = other.head;
            while (current) {
                push_back(current->val);
                current = current->next;
            }
        }
        return *this;
    }

    // basic operations (deque)
    size_t size() const{
        return sz;
    }
    
    void push_back(const std::string&val){
        sz++;
        if (sz == 1) {
            head = tail = new Node(val);
            return;
        }
        tail->next = new Node(val);
        tail->next->prev = tail;
        tail = tail->next;
    }
    
    void push_front(const std::string&val){
        sz++;
        if (sz == 1){
            head = tail = new Node(val);
            return;
        }
        Node *newHead = new Node(val);
        newHead->next = head;
        head->prev = newHead;
        head = newHead;
    }
    
    std::string pop_back(){
        if (sz == 0){
            return "";
        }
        sz--;
        std::string result = tail->val;
        Node *nodeToDelete = tail;
        tail = tail->prev;
        if (tail){
            tail->next = nullptr;
        } else {
            head = nullptr;
        }
        delete nodeToDelete;
        return result;
    }
    
    std::string pop_front(){
        if (sz == 0){
            return "";
        }
        sz--;
        std::string result = head->val;
        Node *nodeToDelete = head;
        head = head->next;
        if (head){
            head->prev = nullptr;
        } else {
            tail = nullptr;
        }
        delete nodeToDelete;
        return result;
    }

    // advance custome operations
    std::vector<std::string> vec() const{
        std::vector<std::string>res;
        Node *tmp = head;
        while (tmp != nullptr){
            res.push_back(tmp->val);
            tmp = tmp->next;
        }
        return res;
    }    
    
    int removeElements(const std::string&value, int count = 1, int dir = 0){
        if (count <= 0) return 0;
        Node *tmp = (dir) ? head : tail;
        int cnt = 0;
        while ((tmp != nullptr) && (cnt < count)){
            Node *next = (dir) ? tmp->next : tmp->prev;
            if (tmp->val == value) {
                deleteNode(tmp); cnt++;
            }
            tmp = next;
        }
        return cnt;
    }

    std::string findAtIndex(int ind){
        if (ind >= ((int)sz)) return "";
        Node *tmp = head;
        for (int i = 0; i < ind; i++){
            tmp = tmp->next;
        }
        return ((tmp != nullptr) ? tmp->val : "");
    }

    bool setAtIndex(int ind, const std::string&value){
        if (ind >= sz) return false;
        Node *tmp = head;
        for (int i = 0; i < ind; i++){
            tmp = tmp->next;
        }
        if (tmp != nullptr) tmp->val = value;
        return (tmp != nullptr);
    }
    // destructor
    ~LList(){
        deleteAll();
    }

};
#endif