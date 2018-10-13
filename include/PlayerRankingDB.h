#pragma once
#ifndef _PLAYER_RANKING_DB_H_
#define _PLAYER_RANKING_DB_H_

#include <string>
#include <vector>


class PlayerRankingDB {
public:
   PlayerRankingDB(void);
   ~PlayerRankingDB();

   void RegisterPlayerResult(std::string playerName, int playerRating);
   void UnregisterPlayer(const std::string& playerName);
   void Rollback(int step);

   int GetPlayerRank(const std::string& playerName) const;

   struct PlayerInfoRow {
      std::string name;
      int         rating;
      int         ranking;

      bool operator==(std::string_view _name) const { return name == _name; }
   };
   std::vector<PlayerInfoRow> GetPlayersInfo(void) const;

private:
   struct Impl;
   std::unique_ptr<Impl> impl;
};


#endif // _PLAYER_RANKING_DB_H_
