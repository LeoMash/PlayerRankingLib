#pragma once

#include "PersistentRedBlackTree.h"
#include <stack>



template <typename Key, typename Val>
template <typename K, typename V>
PersistentRedBlackTree<Key, Val> PersistentRedBlackTree<Key, Val>::insert (K&& key, V&& value) const
{
   auto[mb_new_root, is_new_key] = insert(root, std::forward<K>(key), std::forward<V>(value));
   auto   new_root = mb_new_root->cloneAsBlack();
   size_t new_size = size + (is_new_key ? 1 : 0);

   return PersistentRedBlackTree(new_root, new_size);
}


template <typename Key, typename Val>
template <typename K>
PersistentRedBlackTree<Key, Val> PersistentRedBlackTree<Key, Val>::remove (const K& key) const
{
   auto[mb_new_root, removed] = remove(root, key);
   if (!removed) {
      return *this;
   }

   auto new_root = mb_new_root ? mb_new_root->cloneAsBlack() : mb_new_root;
   return PersistentRedBlackTree(new_root, size - 1);
}


template <typename Key, typename Val>
template <typename K>
auto PersistentRedBlackTree<Key, Val>::get (const K& key) const -> std::optional<entry_type>
{
   auto cur = root;
   while (cur) {
      const key_type& cur_key = cur->key();
      if (key < cur_key) {
         cur = cur->left;
      }
      else if (cur_key < key) {
         cur = cur->right;
      }
      else {
         return *cur->entry;
      }
   }
   return std::nullopt;
}


template <typename Key, typename Val>
auto PersistentRedBlackTree<Key, Val>::balance(const node_ptr_type& node) -> node_ptr_type
{
   assert(node->isBlack());

   // match: (color_l, color_l_l, color_l_r, color_r, color_r_l, color_r_r)

   // case (Some(R), _, _, Some(R), _, _) - both childs are red
   if (isNodeRed(node->left) && isNodeRed(node->right)) {
      auto new_left = node->left ? node->left->cloneAsBlack() : node->left;
      auto new_right = node->right ? node->right->cloneAsBlack() : node->right;

      return Node::makeRed(node->entry, new_left, new_right);
   }

   // case (Some(R), _, _, Opt(B), _, _) - only left child is red
   if (isNodeRed(node->left)) {
      const node_ptr_type& l_l = node->left->left;
      const node_ptr_type& l_r = node->left->right;

      // case: (Some(R), Some(R), _, ...)
      if (isNodeRed(l_l)) {
         auto new_left = Node::makeBlack(l_l->entry, l_l->left, l_l->right);
         auto new_right = Node::makeBlack(node->entry, l_r, node->right);

         return Node::makeRed(node->left->entry, new_left, new_right);
      }

      // case: (Some(R), _, Some(R), ...)
      if (isNodeRed(l_r)) {
         auto new_left = Node::makeBlack(node->left->entry, l_l, l_r->left);
         auto new_right = Node::makeBlack(node->entry, l_r->right, node->right);

         return Node::makeRed(l_r->entry, new_left, new_right);
      }
   }

   // case (Opt(B), _, _, Some(R), _, _) - only right child is red
   if (isNodeRed(node->right)) {
      const node_ptr_type& r_l = node->right->left;
      const node_ptr_type& r_r = node->right->right;

      // case: (..., Some(R), Some(R), _)
      if (isNodeRed(r_l)) {
         auto new_left = Node::makeBlack(node->entry, node->left, r_l->left);
         auto new_right = Node::makeBlack(node->right->entry, r_l->right, r_r);

         return Node::makeRed(r_l->entry, new_left, new_right);
      }

      // case: (..., Some(R), _, Some(R))
      if (isNodeRed(r_r)) {
         auto new_left = Node::makeBlack(node->entry, node->left, r_l);
         auto new_right = Node::makeBlack(r_r->entry, r_r->left, r_r->right);

         return Node::makeRed(node->right->entry, new_left, new_right);
      }
   }

   // case (Opt(B), _, _, Opt(B), _, _) - both childs are already black (or nil)
   return node;
}


template <typename Key, typename Val>
template <typename K, typename V>
auto PersistentRedBlackTree<Key, Val>::insert(const node_ptr_type& node, K&& key, V&& value) -> std::pair<node_ptr_type, bool>
{
   if (node) {
      const key_type& node_key = node->key();
      if (key < node_key) {
         return insertLeft(node, std::forward<K>(key), std::forward<V>(value));
      }
      if (node_key < key) {
         return insertRight(node, std::forward<K>(key), std::forward<V>(value));
      }
      // key == node->key
      entry_ptr_type new_entry = Node::makeEntry(std::forward<K>(key), std::forward<V>(value));
      node_ptr_type new_node = node->cloneWithNewEntry(new_entry);
      return std::make_pair(new_node, false);
   }

   entry_ptr_type new_entry = Node::makeEntry(std::forward<K>(key), std::forward<V>(value));
   node_ptr_type new_node = Node::makeRed(new_entry, nullptr, nullptr);
   return std::make_pair(new_node, true);
}


template <typename Key, typename Val>
template <typename K, typename V>
auto PersistentRedBlackTree<Key, Val>::insertLeft(const node_ptr_type& node, K&& key, V&& value) -> std::pair<node_ptr_type, bool>
{
   auto[new_left, is_new_key] = insert(node->left, std::forward<K>(key), std::forward<V>(value));
   node_ptr_type new_node = node->cloneWithNewLeft(new_left);

   if (is_new_key && new_node->isBlack()) {
      auto balanced_new_node = balance(new_node);
      return std::make_pair(balanced_new_node, is_new_key);
   }

   return std::make_pair(new_node, is_new_key);
}


template <typename Key, typename Val>
template <typename K, typename V>
auto PersistentRedBlackTree<Key, Val>::insertRight(const node_ptr_type& node, K&& key, V&& value) -> std::pair<node_ptr_type, bool>
{
   auto[new_right, is_new_key] = insert(node->right, std::forward<K>(key), std::forward<V>(value));
   node_ptr_type new_node = node->cloneWithNewRight(new_right);

   if (is_new_key && new_node->isBlack()) {
      auto balanced_new_node = balance(new_node);
      return std::make_pair(balanced_new_node, is_new_key);
   }

   return std::make_pair(new_node, is_new_key);
}


template <typename Key, typename Val>
auto PersistentRedBlackTree<Key, Val>::fuse(const node_ptr_type& left, const node_ptr_type& right) -> node_ptr_type
{
   // match: (left, right)
   // case: (None, r)
   if (!left) {
      return right;
   }
   // case: (l, None)
   if (!right) {
      return left;
   }

   // case: (Some(l), Some(r))

   // match: (left.color, right.color)
   bool is_left_red = left->color == Node::Color::RED;
   bool is_right_red = right->color == Node::Color::RED;

   // case: (B, R)
   if (!is_left_red && is_right_red) {
      auto new_left = fuse(left, right->left);
      return Node::makeRed(right->entry, new_left, right->right);
   }

   // case: (R, B)
   if (is_left_red && !is_right_red) {
      auto new_right = fuse(left->right, right);
      return Node::makeRed(left->entry, left->left, new_right);
   }

   // case: (R, R)
   if (is_left_red && is_right_red) {
      auto fused = fuse(left->right, right->left);
      if (isNodeRed(fused)) {
         auto new_left = Node::makeRed(left->entry, left->left, fused->left);
         auto new_right = Node::makeRed(right->entry, fused->right, right->right);

         return Node::makeRed(fused->entry, new_left, new_right);
      }

      auto new_right = Node::makeRed(right->entry, fused, right->right);

      return Node::makeRed(left->entry, left->left, new_right);
   }

   // case: (B, B)
   auto fused = fuse(left->right, right->left);
   if (isNodeRed(fused)) {
      auto new_left = Node::makeBlack(left->entry, left->left, fused->left);
      auto new_right = Node::makeBlack(right->entry, fused->right, right->right);

      return Node::makeRed(fused->entry, new_left, new_right);
   }

   auto new_right = Node::makeBlack(right->entry, fused, right->right);

   auto new_node = Node::makeRed(left->entry, left->left, new_right);
   return balanceRemoveLeft(new_node);
}


template <typename Key, typename Val>
auto PersistentRedBlackTree<Key, Val>::balanceRemoveLeft(const node_ptr_type& node) -> node_ptr_type
{
   // match: (color_l, color_r, color_r_l)
   // case: (Some(R), ..)
   if (isNodeRed(node->left)) {
      auto new_left = Node::makeBlack(node->left->entry, node->left->left, node->left->right);

      return Node::makeRed(node->entry, new_left, node->right);
   }

   // case: (_, Some(B), _)
   if (isNodeBlack(node->right)) {
      auto new_right = Node::makeRed(node->right->entry, node->right->left, node->right->right);

      auto new_node = Node::makeBlack(node->entry, node->left, new_right);
      return balance(new_node);

   }

   // case: (_, Some(R), Some(B))
   assert(isNodeRed(node->right) && isNodeBlack(node->right->left)); // TODO: WHY no other use cases ????
   auto unbalanced_new_right = Node::makeBlack(node->right->entry, node->right->left->right, node->right->right->cloneAsRed());
   auto new_right = balance(unbalanced_new_right);

   auto new_left = Node::makeBlack(node->entry, node->left, node->right->left->left);

   return Node::makeRed(node->right->left->entry, new_left, new_right);
}


template <typename Key, typename Val>
auto PersistentRedBlackTree<Key, Val>::balanceRemoveRight(const node_ptr_type& node) -> node_ptr_type
{
   // match: (color_l, color_l_r, color_r)
   // case: (.., Some(R))
   if (isNodeRed(node->right)) {
      auto new_right = Node::makeBlack(node->right->entry, node->right->left, node->right->right);

      return Node::makeRed(node->entry, node->left, new_right);
   }

   // case: (Some(B), ..)
   if (isNodeBlack(node->left)) {
      auto new_left = Node::makeRed(node->left->entry, node->left->left, node->left->right);

      auto unbalanced_new_node = Node::makeBlack(node->entry, new_left, node->right);
      return balance(unbalanced_new_node);

   }

   // case: (Some(R), Some(B), _)
   assert(isNodeRed(node->left) && isNodeBlack(node->left->right)); // TODO: WHY no other use cases ???? 

   auto unbalanced_new_left = Node::makeBlack(node->left->entry, node->left->left->cloneAsRed(), node->left->right->left);
   auto new_left = balance(unbalanced_new_left);

   auto new_right = Node::makeBlack(node->entry, node->left->right->right, node->right);

   return Node::makeRed(node->left->right->entry, new_left, new_right);

}


template <typename Key, typename Val>
template <typename K>
auto PersistentRedBlackTree<Key, Val>::remove(const node_ptr_type& node, const K& key) -> std::pair<node_ptr_type, bool>
{
   if (node) {
      const key_type& node_key = node->key();
      if (key < node_key) {
         return removeLeft(node, key);
      }
      if (node_key < key) {
         return removeRight(node, key);
      }
      // key == node->key
      auto new_node = fuse(node->left, node->right);
      return std::make_pair(new_node, true);
   }

   return std::make_pair(nullptr, false);
}


template <typename Key, typename Val>
template <typename K>
auto PersistentRedBlackTree<Key, Val>::removeLeft(const node_ptr_type& node, const K& key) -> std::pair<node_ptr_type, bool>
{
   auto[new_left, removed] = remove(node->left, key);

   if (!removed) {
      // node->left must be equal to new_left
      assert(new_left == node->left);
      return std::make_pair(node, false);
   }

   auto new_node = Node::makeRed(node->entry, new_left, node->right);
   if (isNodeBlack(node->left)) {
      auto balanced_new_node = balanceRemoveLeft(new_node);
      return std::make_pair(balanced_new_node, true);
   }

   return std::make_pair(new_node, true);
}


template <typename Key, typename Val>
template <typename K>
auto PersistentRedBlackTree<Key, Val>::removeRight(const node_ptr_type& node, const K& key) -> std::pair<node_ptr_type, bool>
{
   auto[new_right, removed] = remove(node->right, key);

   if (!removed) {
      // node->right must be equal to new_left
      assert(new_right == node->right);
      return std::make_pair(node, false);
   }

   auto new_node = Node::makeRed(node->entry, node->left, new_right);
   if (isNodeBlack(node->right)) {
      auto balanced_new_node = balanceRemoveRight(new_node);
      return std::make_pair(balanced_new_node, true);
   }

   return std::make_pair(new_node, true);
}


template <typename Key, typename Val>
size_t PersistentRedBlackTree<Key, Val>::getBlackHeight(const node_ptr_type& node)
{
   if (!node) {
      // nil node has black height of 1
      return 1;
   }

   const auto& left = node->left;
   const auto& right = node->right;

   if (isNodeRed(node) && (isNodeRed(left) || isNodeRed(right))) {
      // invalid node:
      // - red node must have both childs to be black
      return 0;
   }

   const key_type& node_key = node->key();
   if ((left && left->key() >= node_key) ||
      (right && right->key() <= node_key)) {
      // invalid node:
      // - this tree is not valid search tre
      return 0;
   }

   auto lh = getBlackHeight(left);
   auto rh = getBlackHeight(right);

   if (lh == 0 || rh == 0) {
      // invalid node:
      // - some child is invalid node
      return 0;
   }

   if (lh != rh) {
      // invalid node:
      // - black height of childs is not equal
      return 0;
   }

   // return black height of current node depending on color
   return isNodeRed(node) ? lh : lh + 1;
}



template <typename Key, typename Val>
auto PersistentRedBlackTree<Key, Val>::toMap () const -> std::map<key_type, mapped_type>
{
   std::map<key_type, mapped_type> out;
   auto                            node = root;
   auto                            s = std::stack<node_ptr_type>();
   while (!s.empty() || node) {
      if (node) {
         s.push(node);
         node = node->left;
      }
      else {
         node = s.top();
         s.pop();
         out.emplace(node->entry->first, node->entry->second);
         node = node->right;
      }
   }
   assert(out.size() == getSize());
   return out;
}