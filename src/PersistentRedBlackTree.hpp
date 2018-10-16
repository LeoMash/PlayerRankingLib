#pragma once

#include "PersistentRedBlackTree.h"
#include <stack>



template <typename Key, typename Val, typename Less, template <typename> class NodeMakerT>
template <typename K, typename V>
PersistentRedBlackTree<Key, Val, Less, NodeMakerT> PersistentRedBlackTree<Key, Val, Less, NodeMakerT>::insert (K&& key, V&& value) const
{
   auto[mb_new_root, is_new_key] = insert(root, std::forward<K>(key), std::forward<V>(value));
   auto   new_root = cloneNodeAsBlack(mb_new_root);
   size_t new_size = size + (is_new_key ? 1 : 0);

   return PersistentRedBlackTree(new_root, new_size, nodeMakerFn, lessPred);
}


template <typename Key, typename Val, typename Less, template <typename> class NodeMakerT>
template <typename K>
PersistentRedBlackTree<Key, Val, Less, NodeMakerT> PersistentRedBlackTree<Key, Val, Less, NodeMakerT>::remove (const K& key) const
{
   auto[mb_new_root, removed] = remove(root, key);
   if (!removed) {
      return *this;
   }

   auto new_root = mb_new_root ? cloneNodeAsBlack(mb_new_root) : mb_new_root;
   return PersistentRedBlackTree(new_root, size - 1, nodeMakerFn, lessPred);
}


template <typename Key, typename Val, typename Less, template <typename> class NodeMakerT>
template <typename K>
auto PersistentRedBlackTree<Key, Val, Less, NodeMakerT>::get (const K& key, const lookup_move_cb& lookupCallback) const -> std::optional<Entry>
{
   auto cur = root;
   while (cur) {
      const key_type& cur_key = cur->key();
      if (lessPred(key, cur_key)) {
         if (lookupCallback) {
            lookupCallback(*cur->entry, true);
         }
         cur = cur->left;
      } else if (lessPred(cur_key, key)) {
         if (lookupCallback) {
            lookupCallback(*cur->entry, false);
         }
         cur = cur->right;
      } else {
         return *cur->entry;
      }
   }
   return std::nullopt;
}


template <typename Key, typename Val, typename Less, template <typename> class NodeMakerT>
auto PersistentRedBlackTree<Key, Val, Less, NodeMakerT>::balance(const NodePtr& node) const -> NodePtr
{
   assert(node->isBlack());

   // match: (color_l, color_l_l, color_l_r, color_r, color_r_l, color_r_r)

   // case (Some(R), _, _, Some(R), _, _) - both childs are red
   if (isNodeRed(node->left) && isNodeRed(node->right)) {
      auto new_left = node->left ? cloneNodeAsBlack(node->left) : node->left;
      auto new_right = node->right ? cloneNodeAsBlack(node->right) : node->right;

      return makeNodeRed(node->entry, new_left, new_right);
   }

   // case (Some(R), _, _, Opt(B), _, _) - only left child is red
   if (isNodeRed(node->left)) {
      const NodePtr& l_l = node->left->left;
      const NodePtr& l_r = node->left->right;

      // case: (Some(R), Some(R), _, ...)
      if (isNodeRed(l_l)) {
         auto new_left = makeNodeBlack(l_l->entry, l_l->left, l_l->right);
         auto new_right = makeNodeBlack(node->entry, l_r, node->right);

         return makeNodeRed(node->left->entry, new_left, new_right);
      }

      // case: (Some(R), _, Some(R), ...)
      if (isNodeRed(l_r)) {
         auto new_left = makeNodeBlack(node->left->entry, l_l, l_r->left);
         auto new_right = makeNodeBlack(node->entry, l_r->right, node->right);

         return makeNodeRed(l_r->entry, new_left, new_right);
      }
   }

   // case (Opt(B), _, _, Some(R), _, _) - only right child is red
   if (isNodeRed(node->right)) {
      const NodePtr& r_l = node->right->left;
      const NodePtr& r_r = node->right->right;

      // case: (..., Some(R), Some(R), _)
      if (isNodeRed(r_l)) {
         auto new_left = makeNodeBlack(node->entry, node->left, r_l->left);
         auto new_right = makeNodeBlack(node->right->entry, r_l->right, r_r);

         return makeNodeRed(r_l->entry, new_left, new_right);
      }

      // case: (..., Some(R), _, Some(R))
      if (isNodeRed(r_r)) {
         auto new_left = makeNodeBlack(node->entry, node->left, r_l);
         auto new_right = makeNodeBlack(r_r->entry, r_r->left, r_r->right);

         return makeNodeRed(node->right->entry, new_left, new_right);
      }
   }

   // case (Opt(B), _, _, Opt(B), _, _) - both childs are already black (or nil)
   return node;
}


template <typename Key, typename Val, typename Less, template <typename> class NodeMakerT>
template <typename K, typename V>
auto PersistentRedBlackTree<Key, Val, Less, NodeMakerT>::insert(const NodePtr& node, K&& key, V&& value) const -> std::pair<NodePtr, bool>
{
   if (node) {
      const key_type& node_key = node->key();
      if (lessPred(key, node_key)) {
         return insertLeft(node, std::forward<K>(key), std::forward<V>(value));
      }
      if (lessPred(node_key, key)) {
         return insertRight(node, std::forward<K>(key), std::forward<V>(value));
      }
      // key == node->key
      EntryPtr new_entry = makeEntry(std::forward<K>(key), std::forward<V>(value));
      NodePtr new_node = cloneNodeWithNewEntry(node, new_entry);
      return std::make_pair(new_node, false);
   }

   EntryPtr new_entry = makeEntry(std::forward<K>(key), std::forward<V>(value));
   NodePtr new_node = makeNodeRed(new_entry, nullptr, nullptr);
   return std::make_pair(new_node, true);
}


template <typename Key, typename Val, typename Less, template <typename> class NodeMakerT>
template <typename K, typename V>
auto PersistentRedBlackTree<Key, Val, Less, NodeMakerT>::insertLeft(const NodePtr& node, K&& key, V&& value) const -> std::pair<NodePtr, bool>
{
   auto[new_left, is_new_key] = insert(node->left, std::forward<K>(key), std::forward<V>(value));
   NodePtr new_node = cloneNodeWithNewLeft(node, new_left);

   if (is_new_key && new_node->isBlack()) {
      auto balanced_new_node = balance(new_node);
      return std::make_pair(balanced_new_node, is_new_key);
   }

   return std::make_pair(new_node, is_new_key);
}


template <typename Key, typename Val, typename Less, template <typename> class NodeMakerT>
template <typename K, typename V>
auto PersistentRedBlackTree<Key, Val, Less, NodeMakerT>::insertRight(const NodePtr& node, K&& key, V&& value) const -> std::pair<NodePtr, bool>
{
   auto[new_right, is_new_key] = insert(node->right, std::forward<K>(key), std::forward<V>(value));
   NodePtr new_node = cloneNodeWithNewRight(node, new_right);

   if (is_new_key && new_node->isBlack()) {
      auto balanced_new_node = balance(new_node);
      return std::make_pair(balanced_new_node, is_new_key);
   }

   return std::make_pair(new_node, is_new_key);
}


template <typename Key, typename Val, typename Less, template <typename> class NodeMakerT>
auto PersistentRedBlackTree<Key, Val, Less, NodeMakerT>::fuse(const NodePtr& left, const NodePtr& right) const -> NodePtr
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
      return makeNodeRed(right->entry, new_left, right->right);
   }

   // case: (R, B)
   if (is_left_red && !is_right_red) {
      auto new_right = fuse(left->right, right);
      return makeNodeRed(left->entry, left->left, new_right);
   }

   // case: (R, R)
   if (is_left_red && is_right_red) {
      auto fused = fuse(left->right, right->left);
      if (isNodeRed(fused)) {
         auto new_left = makeNodeRed(left->entry, left->left, fused->left);
         auto new_right = makeNodeRed(right->entry, fused->right, right->right);

         return makeNodeRed(fused->entry, new_left, new_right);
      }

      auto new_right = makeNodeRed(right->entry, fused, right->right);

      return makeNodeRed(left->entry, left->left, new_right);
   }

   // case: (B, B)
   auto fused = fuse(left->right, right->left);
   if (isNodeRed(fused)) {
      auto new_left = makeNodeBlack(left->entry, left->left, fused->left);
      auto new_right = makeNodeBlack(right->entry, fused->right, right->right);

      return makeNodeRed(fused->entry, new_left, new_right);
   }

   auto new_right = makeNodeBlack(right->entry, fused, right->right);

   auto new_node = makeNodeRed(left->entry, left->left, new_right);
   return balanceRemoveLeft(new_node);
}


template <typename Key, typename Val, typename Less, template <typename> class NodeMakerT>
auto PersistentRedBlackTree<Key, Val, Less, NodeMakerT>::balanceRemoveLeft(const NodePtr& node) const -> NodePtr
{
   // match: (color_l, color_r, color_r_l)
   // case: (Some(R), ..)
   if (isNodeRed(node->left)) {
      auto new_left = makeNodeBlack(node->left->entry, node->left->left, node->left->right);

      return makeNodeRed(node->entry, new_left, node->right);
   }

   // case: (_, Some(B), _)
   if (isNodeBlack(node->right)) {
      auto new_right = makeNodeRed(node->right->entry, node->right->left, node->right->right);

      auto new_node = makeNodeBlack(node->entry, node->left, new_right);
      return balance(new_node);

   }

   // case: (_, Some(R), Some(B))
   assert(isNodeRed(node->right) && isNodeBlack(node->right->left));

   auto unbalanced_new_right = makeNodeBlack(node->right->entry, node->right->left->right, cloneNodeAsRed(node->right->right));
   auto new_right = balance(unbalanced_new_right);

   auto new_left = makeNodeBlack(node->entry, node->left, node->right->left->left);

   return makeNodeRed(node->right->left->entry, new_left, new_right);
}


template <typename Key, typename Val, typename Less, template <typename> class NodeMakerT>
auto PersistentRedBlackTree<Key, Val, Less, NodeMakerT>::balanceRemoveRight(const NodePtr& node) const -> NodePtr
{
   // match: (color_l, color_l_r, color_r)
   // case: (.., Some(R))
   if (isNodeRed(node->right)) {
      auto new_right = makeNodeBlack(node->right->entry, node->right->left, node->right->right);

      return makeNodeRed(node->entry, node->left, new_right);
   }

   // case: (Some(B), ..)
   if (isNodeBlack(node->left)) {
      auto new_left = makeNodeRed(node->left->entry, node->left->left, node->left->right);

      auto unbalanced_new_node = makeNodeBlack(node->entry, new_left, node->right);
      return balance(unbalanced_new_node);

   }

   // case: (Some(R), Some(B), _)
   assert(isNodeRed(node->left) && isNodeBlack(node->left->right));

   auto unbalanced_new_left = makeNodeBlack(node->left->entry, cloneNodeAsRed(node->left->left), node->left->right->left);
   auto new_left = balance(unbalanced_new_left);

   auto new_right = makeNodeBlack(node->entry, node->left->right->right, node->right);

   return makeNodeRed(node->left->right->entry, new_left, new_right);

}


template <typename Key, typename Val, typename Less, template <typename> class NodeMakerT>
template <typename K>
auto PersistentRedBlackTree<Key, Val, Less, NodeMakerT>::remove(const NodePtr& node, const K& key) const -> std::pair<NodePtr, bool>
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


template <typename Key, typename Val, typename Less, template <typename> class NodeMakerT>
template <typename K>
auto PersistentRedBlackTree<Key, Val, Less, NodeMakerT>::removeLeft(const NodePtr& node, const K& key) const -> std::pair<NodePtr, bool>
{
   auto[new_left, removed] = remove(node->left, key);

   if (!removed) {
      // node->left must be equal to new_left
      assert(new_left == node->left);
      return std::make_pair(node, false);
   }

   auto new_node = makeNodeRed(node->entry, new_left, node->right);
   if (isNodeBlack(node->left)) {
      auto balanced_new_node = balanceRemoveLeft(new_node);
      return std::make_pair(balanced_new_node, true);
   }

   return std::make_pair(new_node, true);
}


template <typename Key, typename Val, typename Less, template <typename> class NodeMakerT>
template <typename K>
auto PersistentRedBlackTree<Key, Val, Less, NodeMakerT>::removeRight(const NodePtr& node, const K& key) const -> std::pair<NodePtr, bool>
{
   auto[new_right, removed] = remove(node->right, key);

   if (!removed) {
      // node->right must be equal to new_left
      assert(new_right == node->right);
      return std::make_pair(node, false);
   }

   auto new_node = makeNodeRed(node->entry, node->left, new_right);
   if (isNodeBlack(node->right)) {
      auto balanced_new_node = balanceRemoveRight(new_node);
      return std::make_pair(balanced_new_node, true);
   }

   return std::make_pair(new_node, true);
}


template <typename Key, typename Val, typename Less, template <typename> class NodeMakerT>
size_t PersistentRedBlackTree<Key, Val, Less, NodeMakerT>::getBlackHeight(const NodePtr& node)
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



template <typename Key, typename Val, typename Less, template <typename> class NodeMakerT>
auto PersistentRedBlackTree<Key, Val, Less, NodeMakerT>::toMap () const -> std::map<key_type, mapped_type>
{
   std::map<key_type, mapped_type> out;
   auto                            node = root;
   auto                            s = std::stack<NodePtr>();
   while (!s.empty() || node) {
      if (node) {
         s.push(node);
         node = node->left;
      } else {
         node = s.top();
         s.pop();
         out.emplace(node->entry->first, node->entry->second);
         node = node->right;
      }
   }
   assert(out.size() == getSize());
   return out;
}