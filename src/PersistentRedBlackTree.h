#pragma once
#ifndef _PERSISTENT_RED_BLACK_TREE_H_
#define _PERSISTENT_RED_BLACK_TREE_H_

#include <map>
#include <memory>
#include <optional>
#include <utility>
#include <cassert>

template <typename Key, typename Val>
class PersistentRedBlackTree {
public:
   using key_type = Key;
   using mapped_type = Val;
   using entry_type = std::pair<key_type, mapped_type>;

private:
   struct Node;
   using node_ptr_type = std::shared_ptr<const Node>;
   using entry_ptr_type = std::shared_ptr<const entry_type>;

   struct Node : std::enable_shared_from_this<const Node> {
      enum class Color : unsigned char {
         BLACK = 0,
         RED = 1,
      };

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

      template <typename K, typename V>
      static entry_ptr_type makeEntry(K&& key, V&& value)
      {
         return std::make_shared<const entry_type>(std::forward<K>(key), std::forward<V>(value));
      }

      static node_ptr_type make(Color color, const entry_ptr_type& entry, const node_ptr_type& left, const node_ptr_type& right)
      {
         return std::make_shared<const Node>(color, entry, left, right);
      }

      static node_ptr_type makeBlack(const entry_ptr_type& entry, const node_ptr_type& left, const node_ptr_type& right)
      {
         return make(Color::BLACK, entry, left, right);
      }

      static node_ptr_type makeRed(const entry_ptr_type& entry, const node_ptr_type& left, const node_ptr_type& right)
      {
         return make(Color::RED, entry, left, right);
      }

      node_ptr_type cloneWithNewEntry(const entry_ptr_type& new_entry) const
      {
         return make(color, new_entry, left, right);
      }

      node_ptr_type cloneWithNewLeft(const node_ptr_type& new_left) const
      {
         return make(color, entry, new_left, right);
      }

      node_ptr_type cloneWithNewRight(const node_ptr_type& new_right) const
      {
         return make(color, entry, left, new_right);
      }

      node_ptr_type cloneAsBlack() const
      {
         return make(Color::BLACK, entry, left, right);
      }

      node_ptr_type cloneAsRed() const
      {
         return make(Color::RED, entry, left, right);
      }
   };

public:
   PersistentRedBlackTree() = default;
   PersistentRedBlackTree(const PersistentRedBlackTree& other) = default;
   PersistentRedBlackTree(PersistentRedBlackTree&& other) = default;

   PersistentRedBlackTree& operator=(const PersistentRedBlackTree& other) = default;
   PersistentRedBlackTree& operator=(PersistentRedBlackTree&& other) = default;

   template <typename K, typename V>
   PersistentRedBlackTree insert(K&& key, V&& value) const;

   template <typename K>
   PersistentRedBlackTree remove(const K& key) const;

   template <typename K>
   std::optional<entry_type> get(const K& key) const;

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

private:
   PersistentRedBlackTree(node_ptr_type root, std::size_t size)
      : root(root)
      , size(size)
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
   static std::pair<node_ptr_type, bool> insert(const node_ptr_type& node, K&& key, V&& value);
   template <typename K, typename V>
   static std::pair<node_ptr_type, bool> insertLeft(const node_ptr_type& node, K&& key, V&& value);
   template <typename K, typename V>
   static std::pair<node_ptr_type, bool> insertRight(const node_ptr_type& node, K&& key, V&& value);

   template <typename K>
   static std::pair<node_ptr_type, bool> remove(const node_ptr_type& node, const K& key);
   template <typename K>
   static std::pair<node_ptr_type, bool> removeLeft(const node_ptr_type& node, const K& key);
   template <typename K>
   static std::pair<node_ptr_type, bool> removeRight(const node_ptr_type& node, const K& key);

   static node_ptr_type balance(const node_ptr_type& node);

   static node_ptr_type fuse(const node_ptr_type& left, const node_ptr_type& right);
   static node_ptr_type balanceRemoveLeft(const node_ptr_type& node);
   static node_ptr_type balanceRemoveRight(const node_ptr_type& node);

   static size_t getBlackHeight(const node_ptr_type& node);

private:
   node_ptr_type root;
   size_t        size = 0;
};


#include "PersistentRedBlackTree.hpp"

#endif // _PERSISTENT_RED_BLACK_TREE_H_
