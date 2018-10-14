#pragma once
#ifndef _PERSISTENT_RED_BLACK_TREE_H_
#define _PERSISTENT_RED_BLACK_TREE_H_

#include <map>
#include <memory>
#include <optional>
#include <utility>
#include <cassert>
#include <functional>


template <typename Key, typename Val, typename Less = std::less<Key>>
class PersistentRedBlackTree {
public:
   using key_type = Key;
   using mapped_type = Val;
   using less_pred = Less;
   using entry_type = std::pair<key_type, mapped_type>;

   struct Node;
   using node_ptr_type = std::shared_ptr<const Node>;
   using entry_ptr_type = std::shared_ptr<const entry_type>;

   enum class NodeColor : unsigned char {
      BLACK = 0,
      RED = 1,
   };

   using node_maker = std::function<node_ptr_type(NodeColor color, const entry_ptr_type& entry, const node_ptr_type& left, const node_ptr_type& right)>;

private:
   struct Node : std::enable_shared_from_this<const Node> {
      using Color = NodeColor;

      const Color          color;
      const entry_ptr_type entry; // need to store key-value data by pointer to not copy them on node unsharing
      const node_ptr_type  left;
      const node_ptr_type  right;


      Node(Color color, const entry_ptr_type& entry, const node_ptr_type& left, const node_ptr_type& right)
         : color(color)
         , entry(entry)
         , left(left)
         , right(right)
      {
      }

      const key_type& key () const
      {
         return entry->first;
      }

      const mapped_type& value () const
      {
         return entry->second;
      }

      bool isRed() const
      {
         return color == Color::RED;
      }

      bool isBlack() const
      {
         return color == Color::BLACK;
      }
   };

public:
   PersistentRedBlackTree(less_pred pred = less_pred()) : lessPred(pred) {}
   PersistentRedBlackTree(const PersistentRedBlackTree& other) = default;
   PersistentRedBlackTree(PersistentRedBlackTree&& other) = default;

   PersistentRedBlackTree& operator=(const PersistentRedBlackTree& other) = default;
   PersistentRedBlackTree& operator=(PersistentRedBlackTree&& other) = default;

   template <typename K, typename V>
   PersistentRedBlackTree insert(K&& key, V&& value) const;

   template <typename K>
   PersistentRedBlackTree remove(const K& key) const;

   using lookup_move_cb = std::function<void(const entry_type& from, bool toLeft)>;

   template <typename K>
   std::optional<entry_type> get(const K& key, const lookup_move_cb& lookupCallback = lookup_move_cb()) const;

   std::map<key_type, mapped_type> toMap() const;

   size_t getSize() const
   {
      return size;
   }

   void clear()
   {
      root.reset();
   }

   bool isValid() const
   {
      return getBlackHeight(root) != 0;
   }

   template <typename K, typename V>
   static entry_ptr_type makeEntry(K&& key, V&& value)
   {
      return std::make_shared<const entry_type>(std::forward<K>(key), std::forward<V>(value));
   }

   static node_ptr_type makeNodeDefault(NodeColor color, const entry_ptr_type& entry, const node_ptr_type& left, const node_ptr_type& right)
   {
      return std::make_shared<const Node>(color, entry, left, right);
   }

   void setNodeMaker(node_maker fn)
   {
      nodeMakerFn = fn;
   }

private:
   PersistentRedBlackTree(node_ptr_type root, std::size_t size, node_maker nodeMakerFn, less_pred lessPred)
      : root(root)
      , size(size)
      , nodeMakerFn(nodeMakerFn)
      , lessPred(lessPred)
   {
   }

   static bool isNodeRed(const node_ptr_type& node)
   {
      return node && node->color == Node::Color::RED;
   }

   static bool isNodeBlack(const node_ptr_type& node)
   {
      return node && node->color == Node::Color::BLACK;
   }

   template <typename K, typename V>
   std::pair<node_ptr_type, bool> insert(const node_ptr_type& node, K&& key, V&& value) const;
   template <typename K, typename V>
   std::pair<node_ptr_type, bool> insertLeft(const node_ptr_type& node, K&& key, V&& value) const;
   template <typename K, typename V>
   std::pair<node_ptr_type, bool> insertRight(const node_ptr_type& node, K&& key, V&& value) const;

   template <typename K>
   std::pair<node_ptr_type, bool> remove(const node_ptr_type& node, const K& key) const;
   template <typename K>
   std::pair<node_ptr_type, bool> removeLeft(const node_ptr_type& node, const K& key) const;
   template <typename K>
   std::pair<node_ptr_type, bool> removeRight(const node_ptr_type& node, const K& key) const;

   node_ptr_type balance(const node_ptr_type& node) const; 

   node_ptr_type fuse(const node_ptr_type& left, const node_ptr_type& right) const;
   node_ptr_type balanceRemoveLeft(const node_ptr_type& node) const;
   node_ptr_type balanceRemoveRight(const node_ptr_type& node) const;

   static size_t getBlackHeight(const node_ptr_type& node);

   node_ptr_type makeNode(NodeColor color, const entry_ptr_type& entry, const node_ptr_type& left, const node_ptr_type& right) const
   {
      return nodeMakerFn(color, entry, left, right);
   }

   node_ptr_type makeNodeBlack(const entry_ptr_type& entry, const node_ptr_type& left, const node_ptr_type& right) const
   {
      return makeNode(NodeColor::BLACK, entry, left, right);
   }

   node_ptr_type makeNodeRed(const entry_ptr_type& entry, const node_ptr_type& left, const node_ptr_type& right) const
   {
      return makeNode(NodeColor::RED, entry, left, right);
   }

   node_ptr_type cloneNodeWithNewEntry(const node_ptr_type& node, const entry_ptr_type& new_entry) const
   {
      return makeNode(node->color, new_entry, node->left, node->right);
   }

   node_ptr_type cloneNodeWithNewLeft(const node_ptr_type& node, const node_ptr_type& new_left) const
   {
      return makeNode(node->color, node->entry, new_left, node->right);
   }

   node_ptr_type cloneNodeWithNewRight(const node_ptr_type& node, const node_ptr_type& new_right) const
   {
      return makeNode(node->color, node->entry, node->left, new_right);
   }

   node_ptr_type cloneNodeAsBlack(const node_ptr_type& node) const
   {
      return makeNode(NodeColor::BLACK, node->entry, node->left, node->right);
   }

   node_ptr_type cloneNodeAsRed(const node_ptr_type& node) const
   {
      return makeNode(NodeColor::RED, node->entry, node->left, node->right);
   }

private:
   node_ptr_type root;
   size_t        size = 0;
   less_pred     lessPred;
   node_maker    nodeMakerFn = makeNodeDefault;
};


#include "PersistentRedBlackTree.hpp"

#endif // _PERSISTENT_RED_BLACK_TREE_H_
