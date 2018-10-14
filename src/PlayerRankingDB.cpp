#include "PlayerRankingDB.h"

#include <string>
#include <vector>
#include <algorithm>

#include "PersistentRedBlackTree.h"

struct PlayerRankingDB::Impl {
   using PlayersRatings = PersistentRedBlackTree<std::string, int>;
   using PlayersRatingsHistory = std::vector<PlayersRatings>;

   PlayersRatingsHistory playersRatingsHistory{ 1 };

   struct RankingData {
      int numEqualRating;
      int leftSubtreeSize;
   };
   using PlayersRankings = PersistentRedBlackTree<int, RankingData, std::greater<int>>;
   using PlayersRankingsHistory = std::vector<PlayersRankings>;
   PlayersRankingsHistory rankingHistory{ 1 };

   Impl();

   void RegisterPlayerResult(std::string&& playerName, int playerRating);
   void UnregisterPlayer(const std::string& playerName);
   void Rollback(int step);

   int GetPlayerRank(const std::string& playerName) const;

   const PlayersRatings& GetCurrentRatings() const { return playersRatingsHistory.back(); }
   const PlayersRankings& GetCurrentRankings() const { return rankingHistory.back(); }
};


PlayerRankingDB::Impl::Impl ()
{
   auto rankingNodeMakerFn = [] (PlayersRankings::NodeColor color, const PlayersRankings::entry_ptr_type& entry, const PlayersRankings::node_ptr_type& left, const PlayersRankings::node_ptr_type& right) -> PlayersRankings::node_ptr_type {
      int newLeftSubtreeSize = left ? (left->entry->second.leftSubtreeSize + left->entry->second.numEqualRating) : 0;
      PlayersRankings::entry_ptr_type new_entry;
      if (entry->second.leftSubtreeSize != newLeftSubtreeSize) {
         // create new entry with updated value
         new_entry = PlayersRankings::makeEntry(entry->first, RankingData{entry->second.numEqualRating, newLeftSubtreeSize});
      } else {
         new_entry = entry;
      }
      return PlayersRankings::makeNodeDefault(color, new_entry, left, right);
   };
   rankingHistory.back().setNodeMaker(rankingNodeMakerFn);
}


void PlayerRankingDB::Impl::RegisterPlayerResult(std::string&& playerName, int playerRating)
{
   // store or update new player rating information
   playersRatingsHistory.push_back(GetCurrentRatings().insert(std::move(playerName), playerRating));

   int numEqualRanking = 1;
   auto rankingDataOpt = GetCurrentRankings().get(playerRating);
   if (rankingDataOpt) {
      numEqualRanking = rankingDataOpt->second.numEqualRating + 1;
   }

   rankingHistory.push_back(GetCurrentRankings().insert(playerRating, RankingData{numEqualRanking, 0})); // tree sizes will be recalculated on insertion

}


void PlayerRankingDB::Impl::UnregisterPlayer(const std::string& playerName)
{
   // remove player rating information
   auto ratingOpt = GetCurrentRatings().get(playerName);
   if (!ratingOpt) {
      return;
   }
   auto rankingDataOpt = GetCurrentRankings().get(ratingOpt->second);
   assert(rankingDataOpt);
   int numEqualRatingLeft = rankingDataOpt->second.numEqualRating - 1;
   if (numEqualRatingLeft == 0) {
      // remove last entry with such rating
      rankingHistory.push_back(GetCurrentRankings().remove(ratingOpt->second));
   } else {
      // remove node with such rating and reinsert with decreased
      PlayersRankings temp = GetCurrentRankings().remove(ratingOpt->second);
      rankingHistory.push_back(temp.insert(ratingOpt->second, RankingData{numEqualRatingLeft, 0}));
   }

   playersRatingsHistory.push_back(GetCurrentRatings().remove(playerName));
}


void PlayerRankingDB::Impl::Rollback(int step)
{
   assert(step >= 0);
   size_t historyNewSize = std::max(1U, playersRatingsHistory.size() - step);
   playersRatingsHistory.resize(historyNewSize);
   rankingHistory.resize(historyNewSize);
}


int PlayerRankingDB::Impl::GetPlayerRank(const std::string& playerName) const
{
   auto ratingOpt = GetCurrentRatings().get(playerName);
   if (!ratingOpt) {
      return 0;
   }

   int ranking = 0;
   auto lookupCb = [&ranking] (const PlayersRankings::entry_type& entryFrom, bool toLeft) {
      if (!toLeft) {
         ranking += entryFrom.second.leftSubtreeSize + entryFrom.second.numEqualRating;
      }
   };
   auto rankingDataOpt = GetCurrentRankings().get(ratingOpt->second, lookupCb);
   assert(rankingDataOpt);
   ranking += rankingDataOpt->second.leftSubtreeSize;
   
   return ranking + 1; // ranking numeration starts from 1
}


PlayerRankingDB::PlayerRankingDB (void)
   : impl(std::make_unique<Impl>())
{
}


PlayerRankingDB::~PlayerRankingDB ()
{
}


void PlayerRankingDB::RegisterPlayerResult(std::string playerName, int playerRating)
{
   impl->RegisterPlayerResult(std::move(playerName), playerRating);
}


void PlayerRankingDB::UnregisterPlayer(const std::string& playerName)
{
   impl->UnregisterPlayer(playerName);
}


void PlayerRankingDB::Rollback(int step)
{
   impl->Rollback(step);
}


int PlayerRankingDB::GetPlayerRank(const std::string& playerName) const
{
   return impl->GetPlayerRank(playerName);
}


std::vector<PlayerRankingDB::PlayerInfoRow> PlayerRankingDB::GetPlayersInfo (void) const
{
   std::vector<PlayerInfoRow> rows;

   const Impl::PlayersRatings& ratings = impl->GetCurrentRatings();
   rows.reserve(ratings.getSize());

   auto ratingsMap = ratings.toMap();
   for (const auto& ratingInfo : ratingsMap) {
      int ranking = GetPlayerRank(ratingInfo.first) ;
      rows.push_back(PlayerInfoRow{ ratingInfo.first, ratingInfo.second, ranking });
   }

   return rows;
}

