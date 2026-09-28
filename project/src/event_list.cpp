#include "event_list.h"

namespace nano_edr {

EventList::~EventList() {
    ListClear(this);   
}

void ListPushBack(EventList* list, const Event* event) {
    if (list->capacity > 0 && list->size >= list->capacity)
        ListPopFront(list);

    EventNode* node = new EventNode;
    node->event = *event;
    node->next  = nullptr;

    if (list->tail) {
        list->tail->next = node;
    } else {
        list->head= node;
    }
    
    list->tail = node;
    ++list->size;
}

void ListPopFront(EventList* list) {
    if (!list->head) return;

    EventNode* dead = list->head;
    list->head = dead->next;
    delete dead;

    --list->size;
    if (list->size == 0) {
        list->tail = nullptr;
    }
}

void ListClear(EventList* list) {
    while (list->head) {
        ListPopFront(list);
    }
}

} 