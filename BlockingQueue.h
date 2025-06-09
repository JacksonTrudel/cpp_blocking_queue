#include <condition_variable>
#include <iostream>

template <typename T>
class BlockingQueue {
  using Lock = std::unique_lock<std::mutex>;

  struct Node {
    T val;
    Node* next = nullptr;
    Node* prev = nullptr;

    Node(T&& v) : val(std::forward<T>(v)) {}
  };

  struct Stats {
    int waitingForPush_;
    int waitingForPop_;
  };

 public:
  BlockingQueue(int capacity) : capacity_(capacity) {}

  void push(T&& element) noexcept {
    Lock lock(mutex_);

    if (size_ >= capacity_) {
      s_.waitingForPush_++;
      std::cout << s_.waitingForPush_ << " threads waiting to push()"
                << std::endl;
      canPushCv_.wait(lock, [&] { return size_ < capacity_; });
      std::cout << "Unblocked.. pushing" << std::endl;
      s_.waitingForPush_--;
    }

    pushLockedChecked(std::forward<T>(element));

    if (s_.waitingForPop_ > 0) {
      canPopCv_.notify_one();
    }
  }

  T pop() noexcept {
    Lock lock(mutex_);

    if (size_ == 0) {
      s_.waitingForPop_++;
      std::cout << s_.waitingForPop_ << " threads waiting to pop()"
                << std::endl;
      canPopCv_.wait(lock, [&] { return size_ > 0; });
      std::cout << "Unblocked.. popping" << std::endl;
      s_.waitingForPop_--;
    }

    T element = popLockedChecked();

    if (s_.waitingForPush_ > 0) {
      canPushCv_.notify_one();
    }
    return std::move(element);
  }

  int size() const noexcept {
    Lock lock(mutex_);
    return size_;
  }

 private:
  void pushLockedChecked(T&& element) noexcept {
    Node* n = new Node(std::forward<T>(element));
    n->prev = end_;

    if (!front_) {
      front_ = n;
    } else {
      // we must have an end
      end_->next = n;
    }
    end_ = n;
    size_++;
  }

  T popLockedChecked() noexcept {
    Node* toReturn = front_;
    front_ = front_->next;
    if (front_) {
      front_->prev = nullptr;
    }
    if (toReturn == end_) {
      end_ = nullptr;
    }
    T val = std::move(toReturn->val);
    delete toReturn;
    size_--;
    return std::move(val);
  }

  Stats s_;
  int capacity_;
  int size_;
  Node* front_ = nullptr;
  Node* end_ = nullptr;
  std::mutex mutex_;
  std::condition_variable canPushCv_;
  std::condition_variable canPopCv_;
};
