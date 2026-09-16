#pragma once
#include "Block/Block.h"

struct item_stack {
	block_type type = none;
	int count = 0;
};

/*
* the player's hotbar - stacks of blocks picked up by breaking terrain, one of
* which is selected at a time for placing
*/
class Inventory
{
public:
	static const int size = 9;
	item_stack slots[size];
	int selected_slot = 0;

	//stacks onto an existing slot of the same type, otherwise fills the first empty
	//slot. silently dropped if the inventory is full - there's no ground-item system
	void add_item(block_type type) {
		for (int i = 0; i < size; ++i) {
			if (slots[i].type == type) {
				slots[i].count++;
				return;
			}
		}
		for (int i = 0; i < size; ++i) {
			if (slots[i].type == none) {
				slots[i] = { type, 1 };
				return;
			}
		}
	}

	//consumes one of the selected slot's item; false if there was nothing to consume
	bool remove_selected() {
		item_stack& slot = slots[selected_slot];
		if (slot.type == none || slot.count <= 0) return false;
		slot.count--;
		if (slot.count == 0) slot.type = none;
		return true;
	}

	block_type selected_type() const {
		return slots[selected_slot].type;
	}

	void scroll_select(int direction) {
		selected_slot = (selected_slot + direction + size) % size;
	}
};
