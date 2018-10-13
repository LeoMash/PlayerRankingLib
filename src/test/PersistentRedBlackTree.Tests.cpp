#include <gtest/gtest.h>
#include <random>
#include <stack>

#include "PersistentRedBlackTree.h"


using TestTree = PersistentRedBlackTree<int, int>;
using TruthTree = std::map<int, int>;

class PersistentRedBlackTree_Persistence : public ::testing::Test {
protected:
   struct TreePair {
      TestTree tree;
      TruthTree truth;

      // helpers
      void insert(int key, int value)
      {
         tree = tree.insert(key, value);
         truth.emplace(key, value);
      }

      void remove(int key)
      {
         tree = tree.remove(key);
         truth.erase(key);
      }
   };
   using SnapshotsHistory = std::vector<TreePair>;

   void SetUp() override
   {
   }

   void TearDown() override
   {
      checkHistory();
      history.clear();
   }


   void snapshot(const TreePair& state)
   {
      history.push_back(state);
   }

   void checkHistory()
   {
      // verify all snapshots
      for (const TreePair& snapshot : history) {
         ASSERT_TRUE(snapshot.tree.isValid());
         ASSERT_EQ(snapshot.tree.toMap(), snapshot.truth);
      }
   }

   // snapshot history
   SnapshotsHistory history;
};


TEST(PersistentRedBlackTree_Basic, Empty)
{
   TestTree tree;

   ASSERT_TRUE(tree.isValid());
   ASSERT_EQ(0, tree.getSize());
}

TEST_F(PersistentRedBlackTree_Persistence, SingleInsert)
{
   TreePair state;
   snapshot(state);

   state.insert(1, 1);
   snapshot(state);
}

TEST_F(PersistentRedBlackTree_Persistence, SingleRemove)
{
   TreePair state;
   snapshot(state);

   state.insert(1, 1);
   snapshot(state);

   state.remove(1);
   snapshot(state);
}

TEST_F(PersistentRedBlackTree_Persistence, SingleRemoveNonExistent)
{
   TreePair state;
   snapshot(state);

   state.insert(1, 1);
   snapshot(state);

   state.remove(2);
   snapshot(state);
}

TEST_F(PersistentRedBlackTree_Persistence, SequentialInsert)
{
   const int size = 100;

   TreePair state;

   for (int i = 0; i < size; ++i) {
      state.insert(i, i);
      snapshot(state);
   }
}

TEST_F(PersistentRedBlackTree_Persistence, SequentialRemove)
{
   const int size = 100;

   PersistentRedBlackTree<int, int> tree;

   TreePair state;
   for (int i = 0; i < size; ++i) {
      state.insert(i, i);
   }

   for (int i = 0; i < size; ++i) {
      state.remove(i);
      snapshot(state);
   }
}


class PersistentRedBlackTree_Persistence_Param 
   : public PersistentRedBlackTree_Persistence
   , public testing::WithParamInterface<uint32_t > {
};


TEST_P(PersistentRedBlackTree_Persistence_Param, BatchTest)
{
   uint32_t coin_toss = GetParam();
   const int num_snapshots = 100;
   const int num_ops_between_snapshots = 100;


   std::random_device rd;
   std::mt19937 gen{ rd() };
   std::uniform_int_distribution<uint32_t> dis{ 0, 50000 };
   std::uniform_int_distribution<uint32_t> coin{ 0, 100 };

   // on-going state
   TreePair state;

   // build a bunch of snapshots
   for (int i = 0; i < num_snapshots; i++) {
      for (int j = 0; j < num_ops_between_snapshots; j++) {
         int key = dis(gen);
         if (coin(gen) < coin_toss) {
            state.insert(key, key);
         } else {
            state.remove(key);
         }
      }
      snapshot(state);
   }
}

INSTANTIATE_TEST_CASE_P(InsertProbability,
   PersistentRedBlackTree_Persistence_Param,
   testing::Values(25, 50, 75, 100));