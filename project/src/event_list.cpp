#include "event_list.h"

namespace nano_edr {

EventList::~EventList() {
    Clear();
}

void EventList::PushBack(const Event& event) {
    if (capacity_ > 0 && size_ >= capacity_) {
        PopFront();
    }

    EventNode* node = new EventNode(event);

    if (tail_) {
        tail_->next = node;
    } else {
        head_ = node;
    }

    tail_ = node;
    ++size_;
}

void EventList::PopFront() {
    if (!head_) return;

    EventNode* dead = head_;
    head_ = dead->next;
    delete dead;

    --size_;
    if (size_ == 0) {
        tail_ = nullptr;
    }
}

void EventList::Clear() {
    while (head_) {
        PopFront();
    }
}

}  // namespace nano_edr
