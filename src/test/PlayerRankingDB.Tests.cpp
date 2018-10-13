#include <gtest/gtest.h>

#include "PlayerRankingDB.h"


TEST(PlayerRatingsTest, Empty)
{
   PlayerRankingDB db;

   auto rows = db.GetPlayersInfo();
   ASSERT_EQ(0, rows.size());
}


TEST(PlayerRatingsTest, SingleRegister)
{
   PlayerRankingDB db;
   db.RegisterPlayerResult("A", 100);
   
   auto rows = db.GetPlayersInfo();
   ASSERT_EQ(1, rows.size());
   
   auto playerInfo = std::find(rows.begin(), rows.end(), "A");
   ASSERT_NE(rows.end(), playerInfo);
   ASSERT_EQ(100, playerInfo->rating);
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


class PlayerRatingsTest_InitialSetup 
   : public PlayerRatingsTest_InitialSetupBase
   , public ::testing::WithParamInterface<std::vector<DBOperation>> {
protected:
   void SetUp() override
   {
      dbInitialSetup = GetParam();
      PlayerRatingsTest_InitialSetupBase::SetUp();
   }
};


TEST_P(PlayerRatingsTest_InitialSetup, MultipleRegister)
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
   PlayerRatingsTest_InitialSetup,
   ::testing::ValuesIn(_playerRegistrationLists));


class PlayerRatings_RollbackCount_P 
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
      },

      PlayerRatingsTest_InitialSetupBase::SetUp();
   }

};

TEST_P(PlayerRatings_RollbackCount_P, RollbackInserts)
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

TEST_P(PlayerRatings_RollbackCount_P, RollbackRemoves)
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

INSTANTIATE_TEST_CASE_P(Default,
   PlayerRatings_RollbackCount_P,
   ::testing::Range(0, 4)
);
