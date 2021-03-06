
#pragma once
#include "rocksdb/db.h"

namespace rocksdb {

class SnapshotList;

// Snapshots are kept in a doubly-linked list in the DB.
// Each SnapshotImpl corresponds to a particular sequence number.
class SnapshotImpl : public Snapshot {
 public:
  SequenceNumber number_;  // const after creation

  virtual SequenceNumber GetSequenceNumber() const override { return number_; }

 private:
  friend class SnapshotList;

  // SnapshotImpl is kept in a doubly-linked circular list
  SnapshotImpl* prev_;
  SnapshotImpl* next_;

  SnapshotList* list_;                 // just for sanity checks

  int64_t unix_time_;
};

class SnapshotList {
 public:
  SnapshotList() {
    list_.prev_ = &list_;
    list_.next_ = &list_;
    list_.number_ = 0xFFFFFFFFL;      // placeholder marker, for debugging
    count_ = 0;
  }

  bool empty() const { return list_.next_ == &list_; }
  SnapshotImpl* oldest() const { assert(!empty()); return list_.next_; }
  SnapshotImpl* newest() const { assert(!empty()); return list_.prev_; }

  const SnapshotImpl* New(SequenceNumber seq, uint64_t unix_time) {
    SnapshotImpl* s = new SnapshotImpl;
    s->number_ = seq;
    s->unix_time_ = unix_time;
    s->list_ = this;
    s->next_ = &list_;
    s->prev_ = list_.prev_;
    s->prev_->next_ = s;
    s->next_->prev_ = s;
    count_++;
    return s;
  }

  void Delete(const SnapshotImpl* s) {
    assert(s->list_ == this);
    s->prev_->next_ = s->next_;
    s->next_->prev_ = s->prev_;
    count_--;
    delete s;
  }

  // retrieve all snapshot numbers. They are sorted in ascending order.
  void getAll(std::vector<SequenceNumber>& ret) {
    if (empty()) return;
    SnapshotImpl* s = &list_;
    while (s->next_ != &list_) {
      ret.push_back(s->next_->number_);
      s = s ->next_;
    }
  }

  // get the sequence number of the most recent snapshot
  SequenceNumber GetNewest() {
    if (empty()) {
      return 0;
    }
    return newest()->number_;
  }

  int64_t GetOldestSnapshotTime() const {
    if (empty()) {
      return 0;
    } else {
      return oldest()->unix_time_;
    }
  }

  uint64_t count() const { return count_; }

 private:
  // Dummy head of doubly-linked list of snapshots
  SnapshotImpl list_;
  uint64_t count_;
};

}  // namespace rocksdb
