#include <benchmark/benchmark.h>
#include <string>

#include "PlayerRankingDB.h"


static void PlayerRankingBench_Register(benchmark::State& state)
{
   // generate test data
   const int N = (int)state.range(0);

   PlayerRankingDB db;
   for (int j = 0; j < N; ++j) {
      db.RegisterPlayerResult(std::to_string(j), j);
   }

   for (auto _ : state) {
      db.RegisterPlayerResult("AAAA", N);

      state.PauseTiming();
      db.Rollback(1);
      state.ResumeTiming();
   }

   state.SetComplexityN(state.range(0));
}

BENCHMARK(PlayerRankingBench_Register)->RangeMultiplier(8)->Range(1 << 4, 1 << 16)->Complexity(benchmark::oLogN);


static void PlayerRankingBench_Unregister(benchmark::State& state)
{
   // generate test data
   const int N = (int)state.range(0);

   PlayerRankingDB db;
   for (int j = 0; j < N; ++j) {
      db.RegisterPlayerResult(std::to_string(j), j);
   }

   std::string existingItem = std::to_string(N / 2);

   for (auto _ : state) {
      db.UnregisterPlayer(existingItem);

      state.PauseTiming();
      db.Rollback(1);
      state.ResumeTiming();
   }

   state.SetComplexityN(state.range(0));
}

BENCHMARK(PlayerRankingBench_Unregister)->RangeMultiplier(8)->Range(1 << 4, 1 << 16)->Complexity(benchmark::oLogN);


static void PlayerRankingBench_GetRank(benchmark::State& state)
{
   // generate test data
   const int N = (int)state.range(0);

   PlayerRankingDB db;
   for (int j = 0; j < N; ++j) {
      db.RegisterPlayerResult(std::to_string(j), j);
   }

   std::string existingItem = std::to_string(N / 2);

   for (auto _ : state) {
      db.GetPlayerRank(existingItem);
   }

   state.SetComplexityN(state.range(0));
}

BENCHMARK(PlayerRankingBench_GetRank)->RangeMultiplier(8)->Range(1 << 4, 1 << 16)->Complexity(benchmark::oLogN);


static void PlayerRankingBench_RollbackSize(benchmark::State& state)
{
   // generate test data
   const int N = (int)state.range(0);

   PlayerRankingDB db;
   for (int j = 0; j < N; ++j) {
      db.RegisterPlayerResult(std::to_string(j), j);
   }

   std::string existingItem = std::to_string(N / 2);

   for (auto _ : state) {
      state.PauseTiming();
      db.UnregisterPlayer(existingItem);
      state.ResumeTiming();

      db.Rollback(1);
   }

   state.SetComplexityN(state.range(0));
}

BENCHMARK(PlayerRankingBench_RollbackSize)->RangeMultiplier(8)->Range(1 << 4, 1 << 16)->Complexity(benchmark::oLogN);

static void PlayerRankingBench_RollbackStep(benchmark::State& state)
{
   // generate test data
   const int N = 1 << 16;
   const int steps = (int)state.range(0);

   PlayerRankingDB db;
   for (int j = 0; j < N; ++j) {
      db.RegisterPlayerResult(std::to_string(j), j);
   }

   std::string existingItem = std::to_string(N / 2);

   for (auto _ : state) {
      state.PauseTiming();
      for (int i = 0; i < steps; ++i) {
         db.RegisterPlayerResult(std::to_string(i + N), i + N);
      }
      state.ResumeTiming();

      db.Rollback(steps);
   }

   state.SetComplexityN(state.range(0));
}

BENCHMARK(PlayerRankingBench_RollbackStep)->RangeMultiplier(2)->Range(1, 1<<10)->Complexity(benchmark::oN);
