#pragma once
#ifndef _PLAYER_RANKING_DB_H_
#define _PLAYER_RANKING_DB_H_

#include <string>


class PlayerRankingDB {
public:
   PlayerRankingDB(void) = default;

   void RegisterPlayerResult(std::string playerName, int playerRating);
   void UnregisterPlayer(std::string playerName);
   
   int GetPlayerRank(std::string playerName) const;
   int Rollback(int step);
};


#endif // _PLAYER_RANKING_DB_H_
