#pragma once
#include "Block/Block.h"

struct item_stack {
	block_type type = none;
};

/*
* the player's hotbar - whichever slot is selected holds the last block broken
* while it was selected, and can be placed as many times as wanted until a
* different block is broken into that slot or it's dropped
*/
class Inventory
{
public:
	static const int size = 9;
	item_stack slots[size];
	int selected_slot = 0;

	//breaking a block always replaces the selected slot's content with it
	void add_item(block_type type) {
		slots[selected_slot] = { type };
	}

	//clears the selected slot (Q to drop)
	void drop_selected() {
		slots[selected_slot] = { none };
	}

	block_type selected_type() const {
		return slots[selected_slot].type;
	}

	void scroll_select(int direction) {
		selected_slot = (selected_slot + direction + size) % size;
	}
};
