#include <benchmark/benchmark.h>

#include "PersistentRedBlackTree.h"

using TestTree = PersistentRedBlackTree<int, int>;

static void PersistentRedBlackTree_Insert(benchmark::State& state)
{
   // generate test data
   const int N = (int)state.range(0);
   TestTree tree;
   for (int j = 0; j < N; ++j) {
      tree = tree.insert(j, j);
   }

   for (auto _ : state) {
      tree.insert(N, N);
   }

   state.SetComplexityN(state.range(0));
}

BENCHMARK(PersistentRedBlackTree_Insert)->RangeMultiplier(8)->Range(1 << 4, 1 << 16)->Complexity(benchmark::oLogN);


static void PersistentRedBlackTree_Remove(benchmark::State& state)
{
   // generate test data
   const int N = (int)state.range(0);
   TestTree tree;
   for (int j = 0; j < N; ++j) {
      tree = tree.insert(j, j);
   }

   for (auto _ : state) {
      tree.remove(N / 2);
   }

   state.SetComplexityN(state.range(0));
}

BENCHMARK(PersistentRedBlackTree_Remove)->RangeMultiplier(8)->Range(1 << 4, 1 << 16)->Complexity(benchmark::oLogN);
