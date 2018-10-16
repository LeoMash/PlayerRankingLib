#pragma once
#ifndef _PERSISTENT_RED_BLACK_TREE_H_
#define _PERSISTENT_RED_BLACK_TREE_H_

#include <cassert>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <utility>


enum class RedBlackTreeNodeColor : unsigned char {
   BLACK = 0,
   RED = 1,
};


template <typename Node>
struct RedBlackTreeNodeMakerSharedPtr {
   using NodePtr = std::shared_ptr<const Node>;

   template <typename EntryPtr>
   using NodeMakerFn = std::function<NodePtr(RedBlackTreeNodeColor color, const EntryPtr& entry, const NodePtr& left, const NodePtr& right)>;

   template <typename EntryPtr>
   static NodePtr make(RedBlackTreeNodeColor color, const EntryPtr& entry, const NodePtr& left, const NodePtr& right)
   {
      return std::make_shared<const Node>(color, entry, left, right);
   }
};


template <typename Key, typename Val, typename Less = std::less<Key>, template <typename> class NodeMakerT = RedBlackTreeNodeMakerSharedPtr>
class PersistentRedBlackTree {
public:
   using key_type = Key;
   using mapped_type = Val;
   using LessPred = Less;

   using NodeColor = RedBlackTreeNodeColor;

   struct Node;
   using NodeMaker = NodeMakerT<Node>;
   using NodePtr = typename NodeMaker::NodePtr;

   using Entry = std::pair<key_type, mapped_type>;
   using EntryPtr = std::shared_ptr<const Entry>;

   using NodeMakerFn = typename NodeMaker::template NodeMakerFn<EntryPtr>;

private:
   struct Node {
      using Color = RedBlackTreeNodeColor;

      Color    color;
      EntryPtr entry; // need to store key-value data by pointer to not copy them on node unsharing
      NodePtr  left;
      NodePtr  right;

      Node() = default;

      Node(Color color, const EntryPtr& entry, const NodePtr& left, const NodePtr& right)
         : color(color)
         , entry(entry)
         , left(left)
         , right(right)
      {}

      const key_type& key() const
      {
         return entry->first;
      }

      const mapped_type& value() const
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
   PersistentRedBlackTree(NodeMakerFn maker = NodeMaker::template make<EntryPtr>, LessPred pred = LessPred())
      : nodeMakerFn(maker)
      , lessPred(pred)
   {}
   PersistentRedBlackTree(const PersistentRedBlackTree& other) = default;
   PersistentRedBlackTree(PersistentRedBlackTree&& other) = default;

   PersistentRedBlackTree& operator=(const PersistentRedBlackTree& other) = default;
   PersistentRedBlackTree& operator=(PersistentRedBlackTree&& other) = default;

   template <typename K, typename V>
   PersistentRedBlackTree insert(K&& key, V&& value) const;

   template <typename K>
   PersistentRedBlackTree remove(const K& key) const;

   using lookup_move_cb = std::function<void(const Entry& from, bool toLeft)>;

   template <typename K>
   std::optional<Entry> get(const K& key, const lookup_move_cb& lookupCallback = lookup_move_cb()) const;

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
   static EntryPtr makeEntry(K&& key, V&& value)
   {
      return std::make_shared<const Entry>(std::forward<K>(key), std::forward<V>(value));
   }

private:
   PersistentRedBlackTree(NodePtr root, std::size_t size, NodeMakerFn nodeMakerFn, LessPred lessPred)
      : root(root)
      , size(size)
      , nodeMakerFn(nodeMakerFn)
      , lessPred(lessPred)
   {}

   static bool isNodeRed(const NodePtr& node)
   {
      return node && node->color == Node::Color::RED;
   }

   static bool isNodeBlack(const NodePtr& node)
   {
      return node && node->color == Node::Color::BLACK;
   }

   template <typename K, typename V>
   std::pair<NodePtr, bool> insert(const NodePtr& node, K&& key, V&& value) const;
   template <typename K, typename V>
   std::pair<NodePtr, bool> insertLeft(const NodePtr& node, K&& key, V&& value) const;
   template <typename K, typename V>
   std::pair<NodePtr, bool> insertRight(const NodePtr& node, K&& key, V&& value) const;

   template <typename K>
   std::pair<NodePtr, bool> remove(const NodePtr& node, const K& key) const;
   template <typename K>
   std::pair<NodePtr, bool> removeLeft(const NodePtr& node, const K& key) const;
   template <typename K>
   std::pair<NodePtr, bool> removeRight(const NodePtr& node, const K& key) const;

   NodePtr balance(const NodePtr& node) const;

   NodePtr fuse(const NodePtr& left, const NodePtr& right) const;
   NodePtr balanceRemoveLeft(const NodePtr& node) const;
   NodePtr balanceRemoveRight(const NodePtr& node) const;

   static size_t getBlackHeight(const NodePtr& node);

   NodePtr makeNode(NodeColor color, const EntryPtr& entry, const NodePtr& left, const NodePtr& right) const
   {
      return nodeMakerFn(color, entry, left, right);
   }

   NodePtr makeNodeBlack(const EntryPtr& entry, const NodePtr& left, const NodePtr& right) const
   {
      return makeNode(NodeColor::BLACK, entry, left, right);
   }

   NodePtr makeNodeRed(const EntryPtr& entry, const NodePtr& left, const NodePtr& right) const
   {
      return makeNode(NodeColor::RED, entry, left, right);
   }

   NodePtr cloneNodeWithNewEntry(const NodePtr& node, const EntryPtr& new_entry) const
   {
      return makeNode(node->color, new_entry, node->left, node->right);
   }

   NodePtr cloneNodeWithNewLeft(const NodePtr& node, const NodePtr& new_left) const
   {
      return makeNode(node->color, node->entry, new_left, node->right);
   }

   NodePtr cloneNodeWithNewRight(const NodePtr& node, const NodePtr& new_right) const
   {
      return makeNode(node->color, node->entry, node->left, new_right);
   }

   NodePtr cloneNodeAsBlack(const NodePtr& node) const
   {
      return makeNode(NodeColor::BLACK, node->entry, node->left, node->right);
   }

   NodePtr cloneNodeAsRed(const NodePtr& node) const
   {
      return makeNode(NodeColor::RED, node->entry, node->left, node->right);
   }

private:
   NodePtr     root = nullptr;
   size_t      size = 0;
   LessPred    lessPred;
   NodeMakerFn nodeMakerFn;
};


#include "PersistentRedBlackTree.hpp"

#endif // _PERSISTENT_RED_BLACK_TREE_H_
