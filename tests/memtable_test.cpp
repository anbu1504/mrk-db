#include "../include/memtable.hpp"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <vector>

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

        Node* root = m2.getRoot();
        assert(m2.height(root) == 2);
        int balance = m2.getBalance(root);
        assert(balance == 0 || balance == 1 || balance == -1);
        std::cout << "Height and balance pass!" << std::endl;
    }

    void test_LL_rotation() {
        Memtable m(10);
        m.insert(10, 1);
        m.insert(5, 2);
        m.insert(2, 3);  // triggers left-left imbalance

        Node* root = m.getRoot();
        assert(root->key == 5);  // new root after rotateRight
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
        m.insert(5, 3);  // triggers left-right imbalance

        Node* root = m.getRoot();
        assert(root->key == 5);  // new root after LR rotation
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

    void test_getValue() {
        Memtable m(5);
        m.insert(1, 10);
        m.insert(2, 20);
        m.insert(3, 30);

        auto val1 = m.getValue(2);
        assert(val1.has_value());
        assert(val1.value() == 20);
        std::cout << "getValue existing key pass!\n" << std::endl;

        auto val2 = m.getValue(4);
        assert(!val2.has_value());
        std::cout << "getValue non-existing key pass!\n" << std::endl;
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
    void test_scanTree() {
        Memtable m(10);

        m.insert(5, 50);
        m.insert(3, 30);
        m.insert(7, 70);
        m.insert(2, 20);
        m.insert(4, 40);
        m.insert(6, 60);
        m.insert(8, 80);

        auto allEntries = m.scanTree(2, 8);
        assert(allEntries.size() == 7);

        std::cout << "Full range entries:\n" << std::endl;
        for (auto& [key, value] : allEntries) std::cout << key << " -> " << value << std::endl;

        auto partial = m.scanTree(3, 6);
        std::vector<uint64_t> expectedKeys = {3, 4, 5, 6};
        assert(partial.size() == expectedKeys.size());
        for (size_t i = 0; i < expectedKeys.size(); i++) {
            assert(std::get<0>(partial[i]) == expectedKeys[i]);
        }

        std::cout << "Partial range [3,6] pass!\n" << std::endl;

        auto empty = m.scanTree(9, 12);
        assert(empty.empty());
        std::cout << "Empty range [9,12] pass!\n" << std::endl;

        auto single = m.scanTree(5, 5);
        assert(single.size() == 1);
        assert(std::get<0>(single[0]) == 5);
        assert(std::get<1>(single[0]) == 50);
        std::cout << "Single key range [5,5] pass!\n" << std::endl;

        std::cout << "All scanTree tests passed!\n" << std::endl;
    }
    void test_inorderTraversal() {
        Memtable m(10);

        m.insert(5, 50);
        m.insert(3, 30);
        m.insert(7, 70);
        m.insert(2, 20);
        m.insert(4, 40);
        m.insert(6, 60);
        m.insert(8, 80);

        auto entries = m.inorderTraversalDel();

        std::vector<uint64_t> expectedKeys = {2, 3, 4, 5, 6, 7, 8};
        std::vector<uint64_t> expectedValues = {20, 30, 40, 50, 60, 70, 80};

        assert(entries.size() == 2 * expectedKeys.size());

        for (size_t i = 0; i < expectedKeys.size(); ++i) {
            auto key = entries[i * 2];
            auto value = entries[i * 2 + 1];
            assert(key == expectedKeys[i]);
            assert(value == expectedValues[i]);
        }

        assert(m.isEmpty());

        std::cout << "Inorder traversal output:\n";
        for (size_t i = 0; i < expectedKeys.size(); ++i)
            std::cout << entries[i * 2] << " -> " << entries[i * 2 + 1] << std::endl;

        std::cout << "Inorder traversal test passed!\n" << std::endl;
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
    tester.test_getValue();
    tester.test_deleteTree();
    tester.test_scanTree();
    tester.test_inorderTraversal();
    return 0;
}