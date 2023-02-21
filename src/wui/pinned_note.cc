/*
 * Copyright (C) 2022-2023 by the Widelands Development Team
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
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "wui/pinned_note.h"

#include "logic/game_data_error.h"
#include "ui_basic/color_chooser.h"
#include "wui/interactive_player.h"

constexpr int16_t kButtonSize = 32;
constexpr int16_t kSpacing = 4;

PinnedNoteEditor::PinnedNoteEditor(InteractivePlayer& parent,
                                   UI::UniqueWindow::Registry& r,
                                   Widelands::FCoords pos,
                                   const std::string& text,
                                   const RGBColor& rgb,
                                   bool is_new)
   : UI::UniqueWindow(&parent,
                      UI::WindowStyle::kWui,
                      format("pinned_note_%d_%d", pos.x, pos.y),
                      &r,
                      0,
                      0,
                      100,
                      100,
                      _("Pinned Note")),
     iplayer_(parent),
     pos_(pos),
     initial_color_(rgb),
     current_color_(rgb),
     delete_on_cancel_(is_new),

     box_(this, UI::PanelStyle::kWui, 0, 0, UI::Box::Vertical),
     buttons_box_(&box_, UI::PanelStyle::kWui, 0, 0, UI::Box::Horizontal),
     ok_(
        &buttons_box_, "ok", 0, 0, kButtonSize, kButtonSize, UI::ButtonStyle::kWuiPrimary, _("OK")),
     delete_(&buttons_box_,
             "delete",
             0,
             0,
             kButtonSize,
             kButtonSize,
             UI::ButtonStyle::kWuiSecondary,
             _("Delete")),
     cancel_(&buttons_box_,
             "cancel",
             0,
             0,
             kButtonSize,
             kButtonSize,
             UI::ButtonStyle::kWuiSecondary,
             _("Cancel")),
     color_(&buttons_box_,
            "color",
            0,
            0,
            kButtonSize,
            kButtonSize,
            UI::ButtonStyle::kWuiSecondary,
            "",
            _("Change color…")),
     text_(new UI::EditBox(&box_, 0, 0, 400, UI::PanelStyle::kWui)) {
	buttons_box_.add(&delete_, UI::Box::Resizing::kExpandBoth);
	buttons_box_.add_space(kSpacing);
	buttons_box_.add(&cancel_, UI::Box::Resizing::kExpandBoth);
	buttons_box_.add_space(kSpacing);
	buttons_box_.add(&color_, UI::Box::Resizing::kExpandBoth);
	buttons_box_.add_space(kSpacing);
	buttons_box_.add(&ok_, UI::Box::Resizing::kExpandBoth);

	box_.add(text_, UI::Box::Resizing::kExpandBoth);
	box_.add_space(kSpacing);
	box_.add(&buttons_box_, UI::Box::Resizing::kFullSize);

	cancel_.sigclicked.connect([this]() { die(); });
	color_.sigclicked.connect([this]() {
		UI::ColorChooser c(
		   &iplayer_, UI::WindowStyle::kWui, current_color_, &iplayer_.player().get_playercolor());
		if (c.run<UI::Panel::Returncodes>() == UI::Panel::Returncodes::kOk) {
			current_color_ = c.get_color();
			update_color_preview();
		}
	});
	delete_.sigclicked.connect([this]() {
		send_delete();
		die();
	});
	ok_.sigclicked.connect([this]() { ok(); });
	text_->ok.connect([this]() { ok(); });
	text_->cancel.connect([this]() { die(); });

	subscriber_ = Notifications::subscribe<Widelands::NotePinnedNoteMoved>(
	   [this](const Widelands::NotePinnedNoteMoved& note) {
		   if (iplayer_.player_number() == note.player && pos_ == note.old_pos) {
			   pos_ = note.new_pos;
		   }
	   });

	text_->set_text(text);
	update_color_preview();

	set_center_panel(&box_);
	center_to_parent();
	text_->focus();

	initialization_complete();
}

void PinnedNoteEditor::update_color_preview() {
	color_.set_pic(playercolor_image(current_color_, "images/players/team.png"));
}

void PinnedNoteEditor::send_delete() {
	iplayer_.game().send_player_pinned_note(
	   iplayer_.player_number(), pos_, "", current_color_, true);
}

void PinnedNoteEditor::ok() {
	delete_on_cancel_ = false;
	iplayer_.game().send_player_pinned_note(
	   iplayer_.player_number(), pos_, text_->text(), current_color_, false);
	die();
}

void PinnedNoteEditor::die() {
	if (delete_on_cancel_) {
		send_delete();
	}
	UI::UniqueWindow::die();
}

bool PinnedNoteEditor::handle_key(bool down, SDL_Keysym code) {
	if (down) {
		switch (code.sym) {
		case SDLK_RETURN:
			ok();
			return true;
		case SDLK_ESCAPE:
			die();
			return true;
		default:
			break;
		}
	}
	return UI::UniqueWindow::handle_key(down, code);
}

PinnedNoteOverview::PinnedNoteOverview(InteractivePlayer& parent, UI::UniqueWindow::Registry& r)
   : UI::UniqueWindow(&parent,
                      UI::WindowStyle::kWui,
                      "pinned_note_overview",
                      &r,
                      0,
                      0,
                      100,
                      100,
                      _("Pinned Notes")),
     iplayer_(parent),
     box_(this, UI::PanelStyle::kWui, 0, 0, UI::Box::Vertical),
     status_(&box_, UI::PanelStyle::kWui, UI::FontStyle::kWuiLabel, "", UI::Align::kCenter)
{
	set_center_panel(&box_);

	if (get_usedefaultpos()) {
		center_to_parent();
	}

	box_.add(&status_, UI::Box::Resizing::kFullSize);
	think();
	initialization_complete();
}

void PinnedNoteOverview::think() {
	const auto& all_note_pointers = iplayer_.player().all_pinned_notes();
	std::vector<Widelands::PinnedNote*> all_notes;
	bool needs_update = cache_.size() != all_note_pointers.size();

	auto iterator = cache_.begin();
	for (const auto& ptr : all_note_pointers) {
		Widelands::PinnedNote* note = ptr.get(iplayer_.egbase());
		assert(note != nullptr);

		all_notes.push_back(note);

		if (!needs_update) {
			needs_update = (iterator->serial != note->serial()) || (iterator->text != note->get_text());
		}
		++iterator;
	}

	status_.set_text(format(ngettext("%u Pinned Note", "%u Pinned Notes", all_notes.size()), all_notes.size()));

	if (needs_update) {
		cache_.clear();
		box_.clear();
		rows_.clear();

		box_.add(&status_, UI::Box::Resizing::kFullSize);

		for (Widelands::PinnedNote* note : all_notes) {
			box_.add_space(kSpacing);
			PinnedNoteRow* row = new PinnedNoteRow(iplayer_, box_, *note);
			box_.add(row, UI::Box::Resizing::kFullSize);
			rows_.emplace_back(row);
			cache_.emplace_back(note->serial(), note->get_text());
		}

		initialization_complete();
	}
}

PinnedNoteOverview::PinnedNoteRow::PinnedNoteRow(InteractivePlayer& iplayer, UI::Panel& parent, Widelands::PinnedNote& note) :
     UI::Box(&parent, UI::PanelStyle::kWui, 0, 0, UI::Box::Horizontal),
     goto_(this,
             "goto",
             0,
             0,
             kButtonSize,
             kButtonSize,
             UI::ButtonStyle::kWuiSecondary,
             g_image_cache->get("images/wui/menus/goto.png"),
             _("Go to this note’s location")),
     edit_(this,
             "edit",
             0,
             0,
             kButtonSize,
             kButtonSize,
             UI::ButtonStyle::kWuiSecondary,
             g_image_cache->get("images/wui/fieldaction/pinned_note.png"),
             _("Edit this note")),
     delete_(this,
             "delete",
             0,
             0,
             kButtonSize,
             kButtonSize,
             UI::ButtonStyle::kWuiSecondary,
             g_image_cache->get("images/wui/menu_abort.png"),
             _("Delete this note")),
     text_(this, UI::PanelStyle::kWui, UI::FontStyle::kWuiLabel, note.get_text(), UI::Align::kLeft)
{
	goto_.sigclicked.connect([&iplayer, &note]() {
		iplayer.map_view()->scroll_to_field(note.get_position(), MapView::Transition::Smooth);
	});

	delete_.sigclicked.connect([&iplayer, &note]() {
		iplayer.game().send_player_pinned_note(iplayer.player_number(), note.get_position(), "", note.get_rgb(), true);
	});

	edit_.sigclicked.connect([&iplayer, &note]() {
		iplayer.edit_pinned_note(note.get_position());
	});

	text_.set_fixed_width(300);
	add(&goto_, UI::Box::Resizing::kAlign, UI::Align::kCenter);
	add(&edit_, UI::Box::Resizing::kAlign, UI::Align::kCenter);
	add(&delete_, UI::Box::Resizing::kAlign, UI::Align::kCenter);
	add_space(kSpacing);
	add(&text_, UI::Box::Resizing::kAlign, UI::Align::kCenter);
}

constexpr uint16_t kCurrentPacketVersion = 1;
UI::Window& PinnedNoteOverview::load(FileRead& fr, InteractiveBase& ib) {
	try {
		const uint16_t packet_version = fr.unsigned_16();
		if (packet_version == kCurrentPacketVersion) {
			UI::UniqueWindow::Registry& r = dynamic_cast<InteractivePlayer&>(ib).menu_windows_.stats_pinned_notes;
			r.create();
			assert(r.window);
			PinnedNoteOverview& m = dynamic_cast<PinnedNoteOverview&>(*r.window);
			return m;
		}
		throw Widelands::UnhandledVersionError(
		   "Pinned Notes", packet_version, kCurrentPacketVersion);

	} catch (const WException& e) {
		throw Widelands::GameDataError("pinned notes overview: %s", e.what());
	}
}
void PinnedNoteOverview::save(FileWrite& fw, Widelands::MapObjectSaver& /* mos */) const {
	fw.unsigned_16(kCurrentPacketVersion);
	// Nothing to save currently.
}
