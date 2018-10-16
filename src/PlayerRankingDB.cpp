#include "PlayerRankingDB.h"

#include <windows.h>

#ifdef max
#undef max
#endif 

#include <string>
#include <vector>
#include <algorithm>


#include "PersistentRedBlackTree.h"


template <class T>
class BumpAllocator {
public:
   BumpAllocator(size_t reserved, size_t pageSize);
   ~BumpAllocator();

   T* Allocate();
   void ReleaseUpTo(T* ptr) { current = ptr; }

   T* GetCurrent() const { return current; }

private:
   T* current;
   unsigned char* physicalEnd;
   unsigned char* virtualStart;
   unsigned char* virtualEnd;
   size_t growSize;
};


const size_t KB = 1 << 10;
const size_t MB = KB << 10;
const size_t GB = MB << 10;
const size_t TB = GB << 10;


template <class T>
BumpAllocator<T>::BumpAllocator(size_t reserved, size_t growSize)
   : growSize(growSize)
{
   const int PAGE_SIZE = 64 * KB;
   assert(growSize % PAGE_SIZE == 0);
   assert(reserved % growSize == 0);
   // TODO: check alignment

   virtualStart = (unsigned char*)VirtualAlloc(NULL, reserved, MEM_RESERVE, PAGE_READWRITE);
   virtualEnd = virtualStart + reserved;

   physicalEnd = virtualStart;
   current = (T*)physicalEnd;
}


template <class T>
BumpAllocator<T>::~BumpAllocator()
{
   // deconstruct all constructed objects
   T* cur = (T*)virtualStart;
   while ((unsigned char*)(cur + 1) < physicalEnd) {
      cur->~T();
      cur++;
   }

   VirtualFree(virtualStart, 0, MEM_RELEASE);
}


template <class T>
T* BumpAllocator<T>::Allocate()
{
   if ((unsigned char*)(current + 1) > physicalEnd) {
      // not enough physical memory - need to commit more pages
      if (physicalEnd + growSize > virtualEnd) {
         // not enough virtual memory - can't allocate more
         return nullptr;
      }
      if (VirtualAlloc(physicalEnd, growSize, MEM_COMMIT, PAGE_READWRITE) == 0) {
         // allocation failed some how
         return nullptr;
      }
      physicalEnd += growSize;
      // construct objects of T in just allocated memory
      T* cur = current;
      while ((unsigned char*)(cur + 1) < physicalEnd) {
         new (cur) T();
         cur++;
      }
   }

   return current++;
}



template <typename Node>
struct NodeMakerRawPtr {
   using NodePtr = const Node*;

   template <typename EntryPtr>
   using NodeMakerFn = std::function<NodePtr(RedBlackTreeNodeColor color, const EntryPtr& entry, const NodePtr& left, const NodePtr& right)>;

   template <typename EntryPtr>
   static NodePtr make(RedBlackTreeNodeColor color, const EntryPtr& entry, const NodePtr& left, const NodePtr& right)
   {
      return nullptr;
   }
};


struct PlayerRankingDB::Impl {
   using PlayersRatingsTree = PersistentRedBlackTree<std::string, int, std::less<std::string>, NodeMakerRawPtr>;

   struct RankingData {
      int numEqualRating;
      int leftSubtreeSize;
   };
   using PlayersRankingsTree = PersistentRedBlackTree<int, RankingData, std::greater<int>, NodeMakerRawPtr>;

   template <class TreeT>
   struct Snapshot {
      using Node = typename TreeT::Node;
      TreeT tree;
      Node* nodeAllocTop = nullptr;

      Snapshot() = default;
      Snapshot(TreeT&& tree, Node* node) : tree(std::move(tree)), nodeAllocTop(node) {}
   };
   using PlayersRatingsSnapshot = Snapshot<PlayersRatingsTree>;
   using PlayersRankingsSnapshot = Snapshot<PlayersRankingsTree>;

   using PlayersRatingsHistory = std::vector<PlayersRatingsSnapshot>;
   using PlayersRankingsHistory = std::vector<PlayersRankingsSnapshot>;

   BumpAllocator<PlayersRatingsTree::Node> playersRatingsNodeAlloc;
   PlayersRatingsHistory                   playersRatingsHistory;

   BumpAllocator<PlayersRankingsTree::Node> rankingNodeAlloc;
   PlayersRankingsHistory                   rankingHistory;

   Impl();

   void RegisterPlayerResult(std::string&& playerName, int playerRating);
   void UnregisterPlayer(const std::string& playerName);
   void Rollback(int step);

   int GetPlayerRank(const std::string& playerName) const;

   const PlayersRatingsTree& GetCurrentRatings() const { return playersRatingsHistory.back().tree; }
   const PlayersRankingsTree& GetCurrentRankings() const { return rankingHistory.back().tree; }
};


PlayerRankingDB::Impl::Impl ()
   : playersRatingsNodeAlloc(100 * MB, 1 * MB)
   , rankingNodeAlloc(100 * MB, 1 * MB)
{
   auto playerRatingNodeMakerFn = [&] (PlayersRatingsTree::NodeColor color, const PlayersRatingsTree::EntryPtr& entry, const PlayersRatingsTree::NodePtr& left, const PlayersRatingsTree::NodePtr& right) -> PlayersRatingsTree::NodePtr {
      auto* node = playersRatingsNodeAlloc.Allocate();
      node->color = color;
      node->entry = entry;
      node->left = left;
      node->right = right;
      return node;
   };

   playersRatingsHistory.emplace_back(PlayersRatingsTree{ playerRatingNodeMakerFn }, playersRatingsNodeAlloc.GetCurrent());

   auto rankingNodeMakerFn = [&] (PlayersRankingsTree::NodeColor color, const PlayersRankingsTree::EntryPtr& entry, const PlayersRankingsTree::NodePtr& left, const PlayersRankingsTree::NodePtr& right) -> PlayersRankingsTree::NodePtr {
      int newLeftSubtreeSize = left ? (left->entry->second.leftSubtreeSize + left->entry->second.numEqualRating) : 0;
      PlayersRankingsTree::EntryPtr new_entry;
      if (entry->second.leftSubtreeSize != newLeftSubtreeSize) {
         // create new entry with updated value
         new_entry = PlayersRankingsTree::makeEntry(entry->first, RankingData{ entry->second.numEqualRating, newLeftSubtreeSize });
      } else {
         new_entry = entry;
      }

      auto* node = rankingNodeAlloc.Allocate();
      node->color = color;
      node->entry = new_entry;
      node->left = left;
      node->right = right;
      return node;
   };
   rankingHistory.emplace_back(PlayersRankingsTree{ rankingNodeMakerFn }, rankingNodeAlloc.GetCurrent());
}


void PlayerRankingDB::Impl::RegisterPlayerResult(std::string&& playerName, int playerRating)
{
   // store or update new player rating information
   PlayersRatingsTree&& newPlayerRatings = GetCurrentRatings().insert(std::move(playerName), playerRating);
   playersRatingsHistory.emplace_back(std::move(newPlayerRatings), playersRatingsNodeAlloc.GetCurrent());

   int numEqualRanking = 1;
   auto rankingDataOpt = GetCurrentRankings().get(playerRating);
   if (rankingDataOpt) {
      numEqualRanking = rankingDataOpt->second.numEqualRating + 1;
   }

   PlayersRankingsTree&& newPlayerRankings = GetCurrentRankings().insert(playerRating, RankingData{ numEqualRanking, 0 });
   rankingHistory.emplace_back(std::move(newPlayerRankings), rankingNodeAlloc.GetCurrent()); // tree sizes will be recalculated on insertion

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
      PlayersRankingsTree&& newPlayerRankings = GetCurrentRankings().remove(ratingOpt->second);
      rankingHistory.emplace_back(std::move(newPlayerRankings), rankingNodeAlloc.GetCurrent());
   } else {
      // remove node with such rating and reinsert with decreased
      PlayersRankingsTree temp = GetCurrentRankings().remove(ratingOpt->second);
      PlayersRankingsTree&& newPlayerRankings = temp.insert(ratingOpt->second, RankingData{ numEqualRatingLeft, 0 });
      rankingHistory.emplace_back(std::move(newPlayerRankings), rankingNodeAlloc.GetCurrent());
   }

   PlayersRatingsTree&& newPlayerRatings = GetCurrentRatings().remove(playerName);
   playersRatingsHistory.emplace_back(std::move(newPlayerRatings), playersRatingsNodeAlloc.GetCurrent());
}


void PlayerRankingDB::Impl::Rollback(int step)
{
   assert(step >= 0);
   size_t historyNewSize = std::max(1U, playersRatingsHistory.size() - step);

   playersRatingsHistory.resize(historyNewSize);
   playersRatingsNodeAlloc.ReleaseUpTo(playersRatingsHistory.back().nodeAllocTop);

   rankingHistory.resize(historyNewSize);
   rankingNodeAlloc.ReleaseUpTo(rankingHistory.back().nodeAllocTop);
}


int PlayerRankingDB::Impl::GetPlayerRank(const std::string& playerName) const
{
   auto ratingOpt = GetCurrentRatings().get(playerName);
   if (!ratingOpt) {
      return 0;
   }

   int ranking = 0;
   auto lookupCb = [&ranking] (const PlayersRankingsTree::Entry& entryFrom, bool toLeft) {
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
{}


PlayerRankingDB::~PlayerRankingDB ()
{}


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

   const Impl::PlayersRatingsTree& ratings = impl->GetCurrentRatings();
   rows.reserve(ratings.getSize());

   auto ratingsMap = ratings.toMap();
   for (const auto& ratingInfo : ratingsMap) {
      int ranking = GetPlayerRank(ratingInfo.first);
      rows.push_back(PlayerInfoRow{ ratingInfo.first, ratingInfo.second, ranking });
   }

   return rows;
}

