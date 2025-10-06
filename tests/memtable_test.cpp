#include "../include/memtable.hpp"
#include <iostream>
#include <cassert>

class MemtableTester {
public:
    void test_and_insert_size() {
        Memtable m(10);
        assert(m.getSize() == 0);
        m.insert(5, 50);
        m.insert(4, 40);
        m.insert(3, 30);
        assert(m.getSize() == 3);

        std::cout << "Insert pass!" << std::endl;
    }

    void printTree(Node* root, int indent = 0) {
        if (!root) return;
        printTree(root->right, indent + 4);
        std::cout << std::string(indent, ' ') << root->key << " (h=" << root->height << ")\n" << std::endl;
        printTree(root->left, indent + 4);
    }

    bool checkBalance(Memtable& m, Node* node) {
        if (!node) return true;
        int balance = m.getBalance(node);
        if (balance < -1 || balance > 1) return false;
        return checkBalance(m, node->left) && checkBalance(m, node->right);
    }

    void test_height_and_balance() {
        Memtable m2(10);
        m2.insert(10, 1);
        m2.insert(5, 2);
        m2.insert(15, 3);

        Node *root = m2.getRoot();
        assert(m2.height(root) == 2);
        int balance = m2.getBalance(root);
        assert(balance == 0 || balance == 1 || balance == -1);
        std::cout << "Height and balance pass!" << std::endl;
    }

    void test_LL_rotation() {
        Memtable m(10);
        m.insert(10, 1);
        m.insert(5, 2);
        m.insert(2, 3); // triggers left-left imbalance

        Node* root = m.getRoot();
        assert(root->key == 5); // new root after rotateRight
        assert(root->left->key == 2);
        assert(root->right->key == 10);
        assert(checkBalance(m, root));

        std::cout << "Left-Left Rotation pass\n" << std::endl;
    }

    void test_RR_rotation() {
        Memtable m(10);
        m.insert(10, 1);
        m.insert(15, 2);
        m.insert(20, 3); 

        Node* root = m.getRoot();
        assert(root->key == 15); 
        assert(root->left->key == 10);
        assert(root->right->key == 20);
        assert(checkBalance(m, root));

        std::cout << "Right-Right Rotation pass\n" << std::endl;
    }

    void test_LR_rotation() {
        Memtable m(10);
        m.insert(10, 1);
        m.insert(2, 2);
        m.insert(5, 3); // triggers left-right imbalance

        Node* root = m.getRoot();
        assert(root->key == 5); // new root after LR rotation
        assert(root->left->key == 2);
        assert(root->right->key == 10);
        assert(checkBalance(m, root));

        std::cout << "Left-Right Rotation pass\n" << std::endl;
    }

    void test_RL_rotation() {
        Memtable m(10);
        m.insert(10, 1);
        m.insert(20, 2);
        m.insert(15, 3); 

        Node* root = m.getRoot();
        assert(root->key == 15); 
        assert(root->left->key == 10);
        assert(root->right->key == 20);
        assert(checkBalance(m, root));

        std::cout << "Right-Left Rotation pass\n" << std::endl;
    }

    void test_getValue_found() {
        Memtable m(5);
        m.insert(1, 10);
        m.insert(2, 20);
        m.insert(3, 30);

        Node * root = m.getRoot();
        auto opt = root->getValue(2);
        if (opt.has_value()) {
            uint64_t val = opt.value(); 
            std::cout << "Value: " << val << std::endl;
            assert(val == 20);
            std::cout << "Get value passed\n" << std::endl;
        }
        else {
            std::cout << "Key not found\n" << std::endl;
        }
    }

    void test_getValue_notFound() {
        Memtable m(5);
        m.insert(1, 10);
        m.insert(2, 20);
        m.insert(3, 30);

        Node * root = m.getRoot();
        auto opt = root->getValue(4);
        if (opt.has_value()) {
            uint64_t val = opt.value(); 
            std::cout << "Value: " << val << std::endl;
            std::cout << "Get value passed\n" << std::endl;
        }
        else {
            std::cout << "Key not found\n" << std::endl;
            assert(opt == std::nullopt);
            std::cout << "Not Found passed!\n" << std::endl;
        }
    }

    void test_deleteTree() {
        Memtable m(10);
        
        m.insert(10, 1);
        m.insert(5, 2);
        m.insert(15, 3);
        
        assert(m.getSize() == 3);
        
        m.deleteTree();
        
        Node* root = m.getRoot();
        assert(root == nullptr);
        
        assert(m.getSize() == 0);
        
        std::cout << "deleteTree test pass!\n" << std::endl;
    }
};


int main() {
    MemtableTester tester;

    tester.test_and_insert_size();
    tester.test_height_and_balance();
    tester.test_LL_rotation();
    tester.test_RR_rotation();
    tester.test_LR_rotation();
    tester.test_RL_rotation();
    tester.test_getValue_found();
    tester.test_getValue_notFound();
    tester.test_deleteTree();
    return 0;
}