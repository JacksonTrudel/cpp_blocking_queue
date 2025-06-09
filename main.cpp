#include <chrono>
#include <thread>
#include <vector>

#include "BlockingQueue.h"

namespace {

constexpr int kNumPushThreads = 2;

}  // namespace

int main(int argc, char* argv[]) {
  BlockingQueue<int> queue(1 /* capacity */);

  std::vector<std::thread> pushThreads;
  pushThreads.reserve(kNumPushThreads);

  for (int i = 0; i < kNumPushThreads; i++) {
    pushThreads.emplace_back([&queue, offset = i * 1000] {
      using namespace std::chrono_literals;

      int nextMsg = offset;
      while (true) {
        std::this_thread::sleep_for(1s);
        queue.push(nextMsg++);
        std::cout << "Pushed one!" << std::endl;
      }
    });
  }
  std::thread popThread([&queue] {
    using namespace std::chrono_literals;

    while (true) {
      std::this_thread::sleep_for(3s);
      auto val = queue.pop();

      std::cout << "Popped one! " << val << std::endl;
    }
  });

  for (auto& thread : pushThreads) {
    thread.join();
  }
  popThread.join();
  return 0;
}
