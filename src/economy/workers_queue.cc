/*
 * Copyright (C) 2004, 2006-2011 by the Widelands Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "economy/workers_queue.h"

#include "economy/economy.h"
#include "economy/request.h"
#include "io/fileread.h"
#include "io/filewrite.h"
#include "logic/editor_game_base.h"
#include "logic/game.h"
#include "logic/map_objects/tribes/tribe_descr.h"
#include "logic/player.h"
#include "map_io/map_object_loader.h"
#include "map_io/map_object_saver.h"

namespace Widelands {

WorkersQueue::WorkersQueue(PlayerImmovable& init_owner,
                           DescriptionIndex const init_worker,
                           uint8_t const init_max_size)
   : owner_(init_owner),
     worker_type_(init_worker),
     max_capacity_(init_max_size),
     capacity_(init_max_size),
     workers_(),
     request_(nullptr) {
	// Can happen when loading a game
	if (worker_type_ != INVALID_INDEX) {
		update_request();
	}

	// TODO(Notabilis): When set_filled() is called here, a later script call to set the worker of
	// the
	// building will fail. Not sure if this is a bug and/or a bug of this class. I don't really think
	// so.
	/*
	NOCOM(#codereview): For testing, I added this to
	data/tribes/scripting/starting_conditions/barbarians/headquarters.lua

	    prefilled_buildings(player, {"barbarians_barracks", sf.x + 5, sf.y + 5,
	       inputs = {barbarians_carrier = 2}
	    })

	And it works fine. If I set the value higher than allowed, I get a Lua error in my inbox, which
	is correct behaviour.
	Is this the error that you mean, or are you getting something else?

	*/
}

void WorkersQueue::set_capacity(Quantity capacity) {
	assert(capacity <= max_capacity_);
	if (capacity_ != capacity) {
		capacity_ = capacity;
		update_request();
	}
}

void WorkersQueue::drop(Worker& worker) {
	Game& game = dynamic_cast<Game&>(owner().egbase());

	std::vector<Worker*>::iterator it = std::find(workers_.begin(), workers_.end(), &worker);
	if (it == workers_.end()) {
		return;
	}

	workers_.erase(it);

	worker.reset_tasks(game);
	worker.start_task_leavebuilding(game, true);

	update_request();
}

void WorkersQueue::remove_workers(Quantity amount) {
	// NOCOM(Notabilis): Check if there are any resources lost when removing workers
	// Especially if there are any worker-memory-objects left or too much is removed
	// I am not sure about how it should be done, so please check it

	// NOCOM(#codereview): schedule->destroy sould take care of any memory issues.
	// Not sure about the economy().remove_workers myself. If this is called when the worker
	// is consumed to create a soldier, we should remove it from the economy. I better create a
	// recruit
	// so we will get better testing data.

	assert(get_filled() >= amount);

	Game& game = dynamic_cast<Game&>(owner().egbase());

	// Note: This might be slow (removing from start) but we want to consume
	// the first worker in the queue first
	for (Quantity i = 0; i < amount; i++) {
		// Maybe: Remove from economy (I don't think this is required)
		// owner_.economy().remove_workers(worker_type_, amount);
		// Remove worker
		(*(workers_.begin()))->schedule_destroy(game);
		// Remove reference from list
		workers_.erase(workers_.begin());
	}

	update_request();
}

Quantity WorkersQueue::get_filled() const {
	return workers_.size();
}

void WorkersQueue::set_filled(Quantity amount) {

	if (amount > max_capacity()) {
		amount = max_capacity_;
	}
	const size_t currentAmount = get_filled();
	if (amount == currentAmount)
		return;

	// Now adjust them
	while (get_filled() < amount) {
		// Create new worker
		const TribeDescr& tribe = owner().tribe();
		const WorkerDescr* worker_descr = tribe.get_worker_descr(worker_type_);
		EditorGameBase& egbase = owner().egbase();
		Worker& w =
		   worker_descr->create(egbase, owner(), nullptr, owner_.get_positions(egbase).front());
		if (incorporate_worker(egbase, w) == -1) {
			NEVER_HERE();
		}
	}
	if (currentAmount > amount) {
		// Drop workers
		remove_workers(currentAmount - amount);
	}
}

int WorkersQueue::incorporate_worker(EditorGameBase& egbase, Worker& w) {
	if (w.get_location(egbase) != &(owner_)) {
		if (get_filled() + 1 > max_capacity_) {
			return -1;
		}
		w.set_location(&(owner_));
	}

	// Bind the worker into this house, hide him on the map
	if (upcast(Game, game, &egbase)) {
		w.start_task_idle(*game, 0, -1);
	}

	// Not quite sure about next line, the training sites are doing it inside add_worker().
	// But that method is not available for ware/worker-queues.
	// But anyway: Add worker to queue
	workers_.push_back(&w);

	// Make sure the request count is reduced or the request is deleted.
	update_request();
	return 0;
}

void WorkersQueue::remove_from_economy(Economy&) {
	if (worker_type_ != INVALID_INDEX) {
		// Setting request_->economy to nullptr will crash the game on load,
		// but dropping the request and creating a new one in add_to_economy
		// works fine
		if (request_) {
			delete request_;
			request_ = nullptr;
		}
		// Removal of workers from the economy is not required, this is done by the building (or so)
	}
}

void WorkersQueue::add_to_economy(Economy&) {
	if (worker_type_ != INVALID_INDEX) {
		update_request();
	}
}

constexpr uint16_t kCurrentPacketVersion = 2;

void WorkersQueue::write(FileWrite& fw, Game& game, MapObjectSaver& mos) {
	// Adapted copy from WaresQueue
	fw.unsigned_16(kCurrentPacketVersion);

	//  Owner and callback is not saved, but this should be obvious on load.
	fw.c_string(owner().tribe().get_worker_descr(worker_type_)->name().c_str());
	fw.signed_32(max_capacity_);
	fw.signed_32(capacity_);
	if (request_) {
		fw.unsigned_8(1);
		request_->write(fw, game, mos);
	} else {
		fw.unsigned_8(0);
	}
	// Store references to the workers
	fw.unsigned_32(workers_.size());
	for (Worker* w : workers_) {
		assert(mos.is_object_known(*w));
		fw.unsigned_32(mos.get_object_file_index(*w));
	}
}

void WorkersQueue::read(FileRead& fr, Game& game, MapObjectLoader& mol) {
	// Adapted copy from WaresQueue
	uint16_t const packet_version = fr.unsigned_16();
	try {
		if (packet_version == kCurrentPacketVersion) {
			delete request_;
			worker_type_ = owner().tribe().worker_index(fr.c_string());
			max_capacity_ = fr.unsigned_32();
			capacity_ = fr.signed_32();
			assert(capacity_ <= max_capacity_);
			if (fr.unsigned_8()) {
				request_ = new Request(owner_, 0, WorkersQueue::request_callback, wwWORKER);
				request_->read(fr, game, mol);
			} else {
				request_ = nullptr;
			}
			size_t nr_workers = fr.unsigned_32();
			assert(nr_workers <= capacity_);
			assert(workers_.empty());
			for (size_t i = 0; i < nr_workers; ++i) {
				workers_.push_back(&mol.get<Worker>(fr.unsigned_32()));
			}
			assert(workers_.size() == nr_workers);
		} else {
			throw UnhandledVersionError("WorkersQueue", packet_version, kCurrentPacketVersion);
		}
	} catch (const GameDataError& e) {
		throw GameDataError("workersqueue: %s", e.what());
	}
}

void WorkersQueue::request_callback(
   Game& game, Request&, DescriptionIndex const, Worker* const worker, PlayerImmovable& target) {
	WorkersQueue& wq = dynamic_cast<Building&>(target).workersqueue(worker->descr().worker_index());

	assert(worker != nullptr);
	assert(wq.workers_.size() < wq.max_capacity_);
	assert(worker->descr().can_act_as(wq.worker_type_));

	assert(worker->get_location(game) == &target);

	// Update
	wq.incorporate_worker(game, *worker);
}

void WorkersQueue::update_request() {
	assert(worker_type_ != INVALID_INDEX);

	if (workers_.size() < capacity_) {
		if (!request_) {
			request_ = new Request(owner_, worker_type_, WorkersQueue::request_callback, wwWORKER);
		}

		request_->set_count(capacity_ - workers_.size());
		request_->set_exact_match(true);
	} else if (workers_.size() >= capacity_) {
		delete request_;
		request_ = nullptr;

		while (workers_.size() > capacity_) {
			drop(**workers_.rbegin());
		}
	}
}
}
