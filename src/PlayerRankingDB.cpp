#include "PlayerRankingDB.h"

#include <string>
#include <vector>
#include <algorithm>

#include "PersistentRedBlackTree.h"

struct PlayerRankingDB::Impl {
   using PlayersRatings = PersistentRedBlackTree<std::string, int>;
   using PlayersRatingsHistory = std::vector<PlayersRatings>;

   PlayersRatingsHistory playersRatingsHistory{1};
   //PersistentTree<int, int> ranksByRating;

   void RegisterPlayerResult(std::string&& playerName, int playerRating);
   void UnregisterPlayer(const std::string& playerName);
   void Rollback(int step);

   const PlayersRatings& GetCurrentRatings() const { return playersRatingsHistory.back(); }
};


void PlayerRankingDB::Impl::RegisterPlayerResult(std::string&& playerName, int playerRating)
{
   // store or update new player rating information
   playersRatingsHistory.push_back(GetCurrentRatings().insert(std::move(playerName), playerRating));
}


void PlayerRankingDB::Impl::UnregisterPlayer(const std::string& playerName)
{
   // remove player rating information
   playersRatingsHistory.push_back(GetCurrentRatings().remove(playerName));
}


void PlayerRankingDB::Impl::Rollback(int step)
{
   assert(step >= 0);
   size_t historyNewSize = std::max(1U, playersRatingsHistory.size() - step);
   playersRatingsHistory.resize(historyNewSize);
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
   throw std::exception("Not implemented");
}


std::vector<PlayerRankingDB::PlayerInfoRow> PlayerRankingDB::GetPlayersInfo (void) const
{
   std::vector<PlayerInfoRow> rows;

   const Impl::PlayersRatings& ratings = impl->GetCurrentRatings();
   rows.reserve(ratings.getSize());
   
   auto ratingsMap = ratings.toMap();
   for (const auto& ratingInfo: ratingsMap) {
      rows.push_back(PlayerInfoRow{ratingInfo.first, ratingInfo.second, 0});
   }

   return rows;
}

