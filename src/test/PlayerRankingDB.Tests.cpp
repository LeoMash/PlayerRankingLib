#include <gtest/gtest.h>

#include "PlayerRankingDB.h"


TEST(PlayerRatingsTest, Empty)
{
   PlayerRankingDB db;

   auto rows = db.GetPlayersInfo();
   ASSERT_EQ(0, rows.size());
}


using PlayerRegistrationInfo = std::pair<std::string, int>;
enum class DBOperationType {
   REGISTER,
   UNREGISTER,
};

struct DBOperation {
   DBOperationType        type;
   PlayerRegistrationInfo data;

   DBOperation(const PlayerRegistrationInfo& reg) : type(DBOperationType::REGISTER), data(reg) {}
   DBOperation(std::string unreg) : type(DBOperationType::UNREGISTER), data(unreg, 0) {}
};

class PlayerRatingsTest_InitialSetupBase : public ::testing::Test {
protected:
   void SetUp() override
   {
      db = std::make_unique<PlayerRankingDB>();

      for (const auto& op : dbInitialSetup) {
         switch (op.type) {
         case DBOperationType::REGISTER:
            db->RegisterPlayerResult(op.data.first, op.data.second);
            break;
         case DBOperationType::UNREGISTER:
            db->UnregisterPlayer(op.data.first);
            break;
         }
      }
   }

   void TearDown() override
   {
      db.reset();
   }

   std::unique_ptr<PlayerRankingDB> db;
   std::vector<DBOperation> dbInitialSetup;
};


class PlayerRatingsTest_InitialSetup_P 
   : public PlayerRatingsTest_InitialSetupBase
   , public ::testing::WithParamInterface<std::vector<DBOperation>> {
protected:
   void SetUp() override
   {
      dbInitialSetup = GetParam();
      PlayerRatingsTest_InitialSetupBase::SetUp();
   }
};


TEST_P(PlayerRatingsTest_InitialSetup_P, MultipleRegister)
{
   auto rows = db->GetPlayersInfo();
   ASSERT_EQ(dbInitialSetup.size(), rows.size());

   for (const auto& ops : dbInitialSetup) {
      auto playerInfo = std::find(rows.begin(), rows.end(), ops.data.first);
      ASSERT_NE(rows.end(), playerInfo);
      ASSERT_EQ(ops.data.second, playerInfo->rating);
   }
}


const std::vector<DBOperation> _playerRegistrationLists[] = {
   {
      { PlayerRegistrationInfo{"A", 100} },
   },
   {
      { PlayerRegistrationInfo{"A", 100} },
      { PlayerRegistrationInfo{"B", 75} },
      { PlayerRegistrationInfo{"C", 300} },
      { PlayerRegistrationInfo{"D", 15} },
   },
};
INSTANTIATE_TEST_CASE_P(Default, 
   PlayerRatingsTest_InitialSetup_P,
   ::testing::ValuesIn(_playerRegistrationLists));


class PlayerRatingsTest_RollbackCount_P 
   : public PlayerRatingsTest_InitialSetupBase
   , public ::testing::WithParamInterface<int> {
protected:
   void SetUp() override
   {
      dbInitialSetup = {
         { PlayerRegistrationInfo{"A", 100} },
         { PlayerRegistrationInfo{"B", 75} },
         { PlayerRegistrationInfo{"C", 300} },
         { PlayerRegistrationInfo{"D", 15} },
      };

      PlayerRatingsTest_InitialSetupBase::SetUp();
   }
};


TEST_P(PlayerRatingsTest_RollbackCount_P, RollbackInserts)
{
   int numRollback = GetParam();
   db->Rollback(numRollback);

   auto rows = db->GetPlayersInfo();
   int playersNum = dbInitialSetup.size() - numRollback;
   ASSERT_EQ(playersNum, rows.size());

   for (int i = 0; i < playersNum; ++i) {
      const auto& op = dbInitialSetup[i];
      
      auto playerInfo = std::find(rows.begin(), rows.end(), op.data.first);
      ASSERT_NE(rows.end(), playerInfo);
      ASSERT_EQ(op.data.second, playerInfo->rating);
   }
}


TEST_P(PlayerRatingsTest_RollbackCount_P, RollbackRemoves)
{
   // unregister all registered players
   for (const auto& regOp: dbInitialSetup) {
      db->UnregisterPlayer(regOp.data.first);
   }

   int numRollback = GetParam();
   db->Rollback(numRollback);

   auto rows = db->GetPlayersInfo();
   int initialPlayersNum = dbInitialSetup.size();
   int playersNum = numRollback;
   ASSERT_EQ(playersNum, rows.size());

   for (int i = initialPlayersNum - 1; i >= initialPlayersNum - playersNum; --i) {
      const auto& op = dbInitialSetup[i];

      auto playerInfo = std::find(rows.begin(), rows.end(), op.data.first);
      ASSERT_NE(rows.end(), playerInfo);
      ASSERT_EQ(op.data.second, playerInfo->rating);
   }
}


TEST_P(PlayerRatingsTest_RollbackCount_P, Rankings)
{
   int numRollback = GetParam();
   db->Rollback(numRollback);

   const int A_Ranks[] = { 2, 2, 1, 1, 0 };
   EXPECT_EQ(A_Ranks[numRollback], db->GetPlayerRank("A"));
   const int B_Ranks[] = { 3, 3, 2, 0, 0 };
   EXPECT_EQ(B_Ranks[numRollback], db->GetPlayerRank("B"));
   const int C_Ranks[] = { 1, 1, 0, 0, 0 };
   EXPECT_EQ(C_Ranks[numRollback], db->GetPlayerRank("C"));
   const int D_Ranks[] = { 4, 0, 0, 0, 0 };
   EXPECT_EQ(D_Ranks[numRollback], db->GetPlayerRank("D"));
}


INSTANTIATE_TEST_CASE_P(Default,
   PlayerRatingsTest_RollbackCount_P,
   ::testing::Range(0, 4)
);


class PlayerRatingsTest_RepeatedRatings
   : public PlayerRatingsTest_InitialSetupBase {
protected:
   void SetUp() override
   {
      dbInitialSetup = {
         { PlayerRegistrationInfo{"A", 100} },
         { PlayerRegistrationInfo{"B", 75} },
         { PlayerRegistrationInfo{"C", 100} },
         { PlayerRegistrationInfo{"D", 15} },
      };

      PlayerRatingsTest_InitialSetupBase::SetUp();
   }

};


TEST_F(PlayerRatingsTest_RepeatedRatings, BasicCheck)
{
   EXPECT_EQ(1, db->GetPlayerRank("A"));
   EXPECT_EQ(3, db->GetPlayerRank("B"));
   EXPECT_EQ(1, db->GetPlayerRank("C"));
   EXPECT_EQ(4, db->GetPlayerRank("D"));
}


TEST_F(PlayerRatingsTest_RepeatedRatings, RollbackNonUnique)
{
   db->Rollback(2);

   EXPECT_EQ(1, db->GetPlayerRank("A"));
   EXPECT_EQ(2, db->GetPlayerRank("B"));
   EXPECT_EQ(0, db->GetPlayerRank("C"));
   EXPECT_EQ(0, db->GetPlayerRank("D"));
}


TEST_F(PlayerRatingsTest_RepeatedRatings, UnregisterUnique)
{
   db->UnregisterPlayer("B");

   EXPECT_EQ(1, db->GetPlayerRank("A"));
   EXPECT_EQ(0, db->GetPlayerRank("B"));
   EXPECT_EQ(1, db->GetPlayerRank("C"));
   EXPECT_EQ(3, db->GetPlayerRank("D"));
}


TEST_F(PlayerRatingsTest_RepeatedRatings, UnregisterNonUnique)
{
   db->UnregisterPlayer("C");

   EXPECT_EQ(1, db->GetPlayerRank("A"));
   EXPECT_EQ(2, db->GetPlayerRank("B"));
   EXPECT_EQ(0, db->GetPlayerRank("C"));
   EXPECT_EQ(3, db->GetPlayerRank("D"));
}
