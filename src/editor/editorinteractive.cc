/*
 * Copyright (C) 2002-2019 by the Widelands Development Team
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

#include "editor/editorinteractive.h"

#include <memory>
#include <string>
#include <vector>

#include <SDL_keycode.h>
#include <boost/format.hpp>

#include "base/i18n.h"
#include "base/scoped_timer.h"
#include "base/warning.h"
#include "build_info.h"
#include "editor/scripting/builtin.h"
#include "editor/scripting/variable.h"
#include "editor/tools/decrease_height_tool.h"
#include "editor/tools/decrease_resources_tool.h"
#include "editor/tools/increase_height_tool.h"
#include "editor/tools/increase_resources_tool.h"
#include "editor/tools/noise_height_tool.h"
#include "editor/tools/place_critter_tool.h"
#include "editor/tools/place_immovable_tool.h"
#include "editor/tools/set_port_space_tool.h"
#include "editor/tools/set_terrain_tool.h"
#include "editor/ui_menus/help.h"
#include "editor/ui_menus/main_menu_load_map.h"
#include "editor/ui_menus/main_menu_map_options.h"
#include "editor/ui_menus/main_menu_new_map.h"
#include "editor/ui_menus/main_menu_random_map.h"
#include "editor/ui_menus/main_menu_save_map.h"
#include "editor/ui_menus/player_menu.h"
#include "editor/ui_menus/player_teams_menu.h"
#include "editor/ui_menus/scenario_lua.h"
#include "editor/ui_menus/scenario_tool_field_owner_options_menu.h"
#include "editor/ui_menus/scenario_tool_infrastructure_options_menu.h"
#include "editor/ui_menus/scenario_tool_road_options_menu.h"
#include "editor/ui_menus/scenario_tool_vision_options_menu.h"
#include "editor/ui_menus/scenario_tool_worker_options_menu.h"
#include "editor/ui_menus/tool_change_height_options_menu.h"
#include "editor/ui_menus/tool_change_resources_options_menu.h"
#include "editor/ui_menus/tool_noise_height_options_menu.h"
#include "editor/ui_menus/tool_place_critter_options_menu.h"
#include "editor/ui_menus/tool_place_immovable_options_menu.h"
#include "editor/ui_menus/tool_resize_options_menu.h"
#include "editor/ui_menus/tool_set_terrain_options_menu.h"
#include "editor/ui_menus/toolsize_menu.h"
#include "graphic/game_renderer.h"
#include "graphic/graphic.h"
#include "graphic/playercolor.h"
#include "io/filewrite.h"
#include "logic/map.h"
#include "logic/map_objects/tribes/tribes.h"
#include "logic/map_objects/world/resource_description.h"
#include "logic/map_objects/world/world.h"
#include "logic/maptriangleregion.h"
#include "logic/player.h"
#include "logic/playersmanager.h"
#include "map_io/map_loader.h"
#include "map_io/widelands_map_loader.h"
#include "scripting/lua_interface.h"
#include "scripting/lua_table.h"
#include "sound/sound_handler.h"
#include "ui_basic/messagebox.h"
#include "ui_basic/progresswindow.h"
#include "wlapplication_options.h"
#include "wui/game_tips.h"
#include "wui/interactive_base.h"

namespace {
using Widelands::Building;

// Load all tribes from disk.
void load_all_tribes(Widelands::EditorGameBase* egbase) {
	assert(egbase->get_loader_ui());
	egbase->get_loader_ui()->step(_("Loading tribes"));
	egbase->tribes();
}

}  // namespace

EditorInteractive::EditorInteractive(Widelands::EditorGameBase& e)
   : InteractiveBase(e, get_config_section()),
     need_save_(false),
     realtime_(SDL_GetTicks()),
     is_painting_(false),
     finalized_(false),
     illustrating_vision_for_(0),
     mainmenu_(toolbar(),
               "dropdown_menu_main",
               0,
               0,
               34U,
               10,
               34U,
               /** TRANSLATORS: Title for the main menu button in the editor */
               as_tooltip_text_with_hotkey(_("Main Menu"), pgettext("hotkey", "Esc")),
               UI::DropdownType::kPictorialMenu,
               UI::PanelStyle::kWui,
               UI::ButtonStyle::kWuiPrimary),
     toolmenu_(toolbar(),
               "dropdown_menu_tools",
               0,
               0,
               34U,
               12,
               34U,
               /** TRANSLATORS: Title for the tool menu button in the editor */
               as_tooltip_text_with_hotkey(_("Map Tools"), "T"),
               UI::DropdownType::kPictorialMenu,
               UI::PanelStyle::kWui,
               UI::ButtonStyle::kWuiPrimary),
     scenario_toolmenu_(toolbar(),
                        "dropdown_menu_scenario_tools",
                        0,
                        0,
                        34U,
                        12,
                        34U,
                        as_tooltip_text_with_hotkey(_("Scenario Tools"), "S"),
                        UI::DropdownType::kPictorialMenu,
                        UI::PanelStyle::kWui,
                        UI::ButtonStyle::kWuiPrimary),
     showhidemenu_(toolbar(),
                   "dropdown_menu_showhide",
                   0,
                   0,
                   34U,
                   10,
                   34U,
                   /** TRANSLATORS: Title for a menu button in the editor. This menu will show/hide
                      building spaces, animals, immovables, resources */
                   _("Show / Hide"),
                   UI::DropdownType::kPictorialMenu,
                   UI::PanelStyle::kWui,
                   UI::ButtonStyle::kWuiPrimary),
     undo_(nullptr),
     redo_(nullptr),
     tools_(new Tools(e.map())),
     history_(nullptr)  // history needs the undo/redo buttons
{
	unfinalize();

	add_main_menu();
	add_tool_menu();
	add_scenario_tool_menu();

	add_toolbar_button(
	   "wui/editor/menus/toolsize", "toolsize", _("Tool size"), &menu_windows_.toolsize, true);
	menu_windows_.toolsize.open_window = [this] {
		new EditorToolsizeMenu(*this, menu_windows_.toolsize);
	};

	toolbar()->add_space(15);

	add_mapview_menu(MiniMapType::kStaticMap);
	add_showhide_menu();

	toolbar()->add_space(15);

	undo_ = add_toolbar_button("wui/editor/menus/undo", "undo", _("Undo"));
	redo_ = add_toolbar_button("wui/editor/menus/redo", "redo", _("Redo"));

	history_.reset(new EditorHistory(*undo_, *redo_));

	undo_->sigclicked.connect([this] { history_->undo_action(); });
	redo_->sigclicked.connect([this] { history_->redo_action(); });

	toolbar()->add_space(15);

	add_toolbar_button("ui_basic/menu_help", "help", _("Help"), &menu_windows_.help, true);
	menu_windows_.help.open_window = [this] {
		new EditorHelp(*this, menu_windows_.help, &egbase().lua());
	};

	finalize_toolbar();

#ifndef NDEBUG
	set_display_flag(InteractiveBase::dfDebug, true);
#else
	set_display_flag(InteractiveBase::dfDebug, false);
#endif

	map_view()->field_clicked.connect([this](const Widelands::NodeAndTriangle<>& node_and_triangle) {
		map_clicked(node_and_triangle, false);
	});

	player_notes_ = Notifications::subscribe<Widelands::NoteEditorPlayerEdited>(
	   [this](const Widelands::NoteEditorPlayerEdited& n) {
		   if (n.map == &egbase().map()) {
			   update_players();
		   }
		});

	update_players();
}

void EditorInteractive::add_main_menu() {
	mainmenu_.set_image(g_gr->images().get("images/wui/editor/menus/main_menu.png"));
	menu_windows_.newmap.open_window = [this] { new MainMenuNewMap(*this, menu_windows_.newmap); };
	/** TRANSLATORS: An entry in the editor's main menu */
	mainmenu_.add(_("New Map"), MainMenuEntry::kNewMap,
	              g_gr->images().get("images/wui/editor/menus/new_map.png"));

	menu_windows_.newrandommap.open_window = [this] {
		new MainMenuNewRandomMap(*this, menu_windows_.newrandommap);
	};
	/** TRANSLATORS: An entry in the editor's main menu */
	mainmenu_.add(_("New Random Map"), MainMenuEntry::kNewRandomMap,
	              g_gr->images().get("images/wui/editor/menus/new_random_map.png"));

	menu_windows_.loadmap.open_window = [this] {
		new MainMenuLoadMap(*this, menu_windows_.loadmap);
	};
	/** TRANSLATORS: An entry in the editor's main menu */
	mainmenu_.add(_("Load Map"), MainMenuEntry::kLoadMap,
	              g_gr->images().get("images/wui/editor/menus/load_map.png"), false, "",
	              pgettext("hotkey", "Ctrl+L"));

	menu_windows_.savemap.open_window = [this] {
		new MainMenuSaveMap(*this, menu_windows_.savemap, menu_windows_.mapoptions);
	};
	/** TRANSLATORS: An entry in the editor's main menu */
	mainmenu_.add(_("Save Map"), MainMenuEntry::kSaveMap,
	              g_gr->images().get("images/wui/editor/menus/save_map.png"), false, "",
	              pgettext("hotkey", "Ctrl+S"));

	menu_windows_.mapoptions.open_window = [this] {
		new MainMenuMapOptions(*this, menu_windows_.mapoptions);
	};
	/** TRANSLATORS: An entry in the editor's main menu */
	mainmenu_.add(_("Map Options"), MainMenuEntry::kMapOptions,
	              g_gr->images().get("images/wui/editor/menus/map_options.png"));

	/** TRANSLATORS: An entry in the editor's main menu */
	mainmenu_.add(_("Exit Editor"), MainMenuEntry::kExitEditor,
	              g_gr->images().get("images/wui/menus/exit.png"));
	mainmenu_.selected.connect([this] { main_menu_selected(mainmenu_.get_selected()); });
	toolbar()->add(&mainmenu_);
}

void EditorInteractive::main_menu_selected(MainMenuEntry entry) {
	switch (entry) {
	case MainMenuEntry::kNewMap: {
		menu_windows_.newmap.toggle();
	} break;
	case MainMenuEntry::kNewRandomMap: {
		menu_windows_.newrandommap.toggle();
	} break;
	case MainMenuEntry::kLoadMap: {
		menu_windows_.loadmap.toggle();
	} break;
	case MainMenuEntry::kSaveMap: {
		menu_windows_.savemap.toggle();
	} break;
	case MainMenuEntry::kMapOptions: {
		menu_windows_.mapoptions.toggle();
	} break;
	case MainMenuEntry::kExitEditor: {
		exit();
	}
	}
}

void EditorInteractive::add_tool_menu() {
	toolmenu_.set_image(g_gr->images().get("images/wui/editor/menus/tools.png"));

	tool_windows_.height.open_window = [this] {
		new EditorToolChangeHeightOptionsMenu(*this, tools()->increase_height, tool_windows_.height);
	};
	/** TRANSLATORS: An entry in the editor's tool menu */
	toolmenu_.add(_("Change height"), ToolMenuEntry::kChangeHeight,
	              g_gr->images().get("images/wui/editor/tools/height.png"), false,
	              /** TRANSLATORS: Tooltip for the change height tool in the editor */
	              _("Change the terrain height"));

	tool_windows_.noiseheight.open_window = [this] {
		new EditorToolNoiseHeightOptionsMenu(*this, tools()->noise_height, tool_windows_.noiseheight);
	};
	/** TRANSLATORS: An entry in the editor's tool menu */
	toolmenu_.add(_("Random height"), ToolMenuEntry::kRandomHeight,
	              g_gr->images().get("images/wui/editor/tools/noise_height.png"), false,
	              /** TRANSLATORS: Tooltip for the random height tool in the editor */
	              _("Set the terrain height to random values"));

	tool_windows_.terrain.open_window = [this] {
		new EditorToolSetTerrainOptionsMenu(*this, tools()->set_terrain, tool_windows_.terrain);
	};
	/** TRANSLATORS: An entry in the editor's tool menu */
	toolmenu_.add(_("Terrain"), ToolMenuEntry::kTerrain,
	              g_gr->images().get("images/wui/editor/tools/terrain.png"), false,
	              /** TRANSLATORS: Tooltip for the terrain tool in the editor */
	              _("Change the map’s terrain"));

	tool_windows_.immovables.open_window = [this] {
		new EditorToolPlaceImmovableOptionsMenu(
		   *this, tools()->place_immovable, tool_windows_.immovables);
	};
	/** TRANSLATORS: An entry in the editor's tool menu */
	toolmenu_.add(_("Immovables"), ToolMenuEntry::kImmovables,
	              g_gr->images().get("images/wui/editor/tools/immovables.png"), false,
	              /** TRANSLATORS: Tooltip for the immovables tool in the editor */
	              _("Add or remove immovables"));

	tool_windows_.critters.open_window = [this] {
		new EditorToolPlaceCritterOptionsMenu(*this, tools()->place_critter, tool_windows_.critters);
	};
	/** TRANSLATORS: An entry in the editor's tool menu */
	toolmenu_.add(_("Animals"), ToolMenuEntry::kAnimals,
	              g_gr->images().get("images/wui/editor/tools/critters.png"), false,
	              /** TRANSLATORS: Tooltip for the animals tool in the editor */
	              _("Add or remove animals"));

	tool_windows_.resources.open_window = [this] {
		new EditorToolChangeResourcesOptionsMenu(
		   *this, tools()->increase_resources, tool_windows_.resources);
	};
	/** TRANSLATORS: An entry in the editor's tool menu */
	toolmenu_.add(_("Resources"), ToolMenuEntry::kResources,
	              g_gr->images().get("images/wui/editor/tools/resources.png"), false,
	              /** TRANSLATORS: Tooltip for the resources tool in the editor */
	              _("Set or change resources"));

	/** TRANSLATORS: An entry in the editor's tool menu */
	toolmenu_.add(_("Port spaces"), ToolMenuEntry::kPortSpace,
	              g_gr->images().get("images/wui/editor/tools/port_spaces.png"), false,
	              /** TRANSLATORS: Tooltip for the port spaces tool in the editor */
	              _("Add or remove port spaces"));

	tool_windows_.players.open_window = [this] {
		if (finalized_)
			new EditorPlayerTeamsMenu(*this, tools()->info, tool_windows_.players);
		else
			new EditorPlayerMenu(*this, tools()->set_starting_pos, tool_windows_.players);
	};
	/** TRANSLATORS: An entry in the editor's tool menu */
	toolmenu_.add(_("Players"), ToolMenuEntry::kPlayers,
	              g_gr->images().get("images/wui/editor/tools/players.png"), false,
	              finalized_ ?
	                 /** TRANSLATORS: Tooltip for the players tool in the editor */
	                 _("Assign players to teams, set hostility relations, and configure allowed "
	                   "building types") :
	                 /** TRANSLATORS: Tooltip for the players tool in the editor */
	                 _("Set number of players and their names, tribes and starting positions"),
	              "P");

	/** TRANSLATORS: An entry in the editor's tool menu */
	toolmenu_.add(_("Map origin"), ToolMenuEntry::kMapOrigin,
	              g_gr->images().get("images/wui/editor/tools/map_origin.png"), false,
	              /** TRANSLATORS: Tooltip for the map origin tool in the editor */
	              _("Set the position that will have the coordinates (0, 0). This will be the "
	                "top-left corner of a generated minimap."));

	tool_windows_.resizemap.open_window = [this] {
		new EditorToolResizeOptionsMenu(*this, tools()->resize, tool_windows_.resizemap);
	};
	/** TRANSLATORS: An entry in the editor's tool menu */
	toolmenu_.add(_("Map size"), ToolMenuEntry::kMapSize,
	              g_gr->images().get("images/wui/editor/tools/resize_map.png"), false,
	              /** TRANSLATORS: Tooltip for the map size tool in the editor */
	              _("Change the map’s size"));

	/** TRANSLATORS: An entry in the editor's tool menu */
	toolmenu_.add(_("Information"), ToolMenuEntry::kFieldInfo,
	              g_gr->images().get("images/wui/editor/fsel_editor_info.png"), false,
	              /** TRANSLATORS: Tooltip for the map information tool in the editor */
	              _("Click on a field to show information about it"), "I");
	toolmenu_.selected.connect([this] { tool_menu_selected(toolmenu_.get_selected()); });
	toolbar()->add(&toolmenu_);
}

void EditorInteractive::tool_menu_selected(ToolMenuEntry entry) {
	switch (entry) {
	case ToolMenuEntry::kChangeHeight:
		tool_windows_.height.toggle();
		break;
	case ToolMenuEntry::kRandomHeight:
		tool_windows_.noiseheight.toggle();
		break;
	case ToolMenuEntry::kTerrain:
		tool_windows_.terrain.toggle();
		break;
	case ToolMenuEntry::kImmovables:
		tool_windows_.immovables.toggle();
		break;
	case ToolMenuEntry::kAnimals:
		tool_windows_.critters.toggle();
		break;
	case ToolMenuEntry::kResources:
		tool_windows_.resources.toggle();
		break;
	case ToolMenuEntry::kPortSpace:
		select_tool(tools()->set_port_space, EditorTool::First);
		break;
	case ToolMenuEntry::kPlayers:
		tool_windows_.players.toggle();
		break;
	case ToolMenuEntry::kMapOrigin:
		select_tool(tools()->set_origin, EditorTool::First);
		break;
	case ToolMenuEntry::kMapSize:
		tool_windows_.resizemap.toggle();
		break;
	case ToolMenuEntry::kFieldInfo:
		select_tool(tools()->info, EditorTool::First);
		break;
	}
	toolmenu_.toggle();
}

void EditorInteractive::add_scenario_tool_menu() {
	scenario_toolmenu_.set_image(g_gr->images().get("images/wui/editor/menus/scenario_tools.png"));
	rebuild_scenario_tool_menu();
	scenario_toolmenu_.selected.connect(
	   [this] { scenario_tool_menu_selected(scenario_toolmenu_.get_selected()); });
	toolbar()->add(&scenario_toolmenu_);
}

void EditorInteractive::rebuild_scenario_tool_menu() {
	scenario_toolmenu_.clear();

	if (!finalized_) {
		/** TRANSLATORS: An entry in the editor's scenario tool menu */
		scenario_toolmenu_.add(_("Enable Scenario Functions…"), ScenarioToolMenuEntry::kFinalize,
		                       g_gr->images().get("images/wui/editor/menus/scenario.png"));
		return;
	}

	scenario_tool_windows_.fieldowner.open_window = [this] {
		new ScenarioToolFieldOwnerOptionsMenu(
		   *this, tools()->sc_owner, scenario_tool_windows_.fieldowner);
	};
	scenario_toolmenu_.add(
	   /** TRANSLATORS: An entry in the editor's scenario tool menu */
	   _("Ownership"), ScenarioToolMenuEntry::kFieldOwner,
	   g_gr->images().get("images/wui/editor/tools/sc_owner.png"), false,
	   /** TRANSLATORS: Tooltip for the field ownership scenario tool in the editor */
	   _("Set the initial ownership of fields"));

	scenario_tool_windows_.vision.open_window = [this] {
		new ScenarioToolVisionOptionsMenu(*this, tools()->sc_vision, scenario_tool_windows_.vision);
	};
	scenario_toolmenu_.add(
	   /** TRANSLATORS: An entry in the editor's scenario tool menu */
	   _("Vision"), ScenarioToolMenuEntry::kVision,
	   g_gr->images().get("images/wui/editor/tools/sc_vis.png"), false,
	   /** TRANSLATORS: Tooltip for the vision scenario tool in the editor */
	   _("Reveal and hide fields to the players"));

	scenario_tool_windows_.infrastructure.open_window = [this] {
		new ScenarioToolInfrastructureOptionsMenu(
		   *this, tools()->sc_infra, scenario_tool_windows_.infrastructure);
	};
	scenario_toolmenu_.add(
	   /** TRANSLATORS: An entry in the editor's scenario tool menu */
	   _("Place Infrastructure"), ScenarioToolMenuEntry::kInfrastructure,
	   g_gr->images().get("images/wui/editor/tools/sc_infra.png"), false,
	   /** TRANSLATORS: Tooltip for the place infrastructure scenario tool in the editor */
	   _("Place buildings, flags and tribe immovables"));

	scenario_tool_windows_.worker.open_window = [this] {
		new ScenarioToolWorkerOptionsMenu(*this, tools()->sc_worker, scenario_tool_windows_.worker);
	};
	scenario_toolmenu_.add(
	   /** TRANSLATORS: An entry in the editor's scenario tool menu */
	   _("Workers and Ships"), ScenarioToolMenuEntry::kWorker,
	   g_gr->images().get("images/wui/editor/tools/sc_worker.png"), false,
	   /** TRANSLATORS: Tooltip for the place workers scenario tool in the editor */
	   _("Place workers, ships and ferries on the map"));

	scenario_tool_windows_.road.open_window = [this] {
		new ScenarioToolRoadOptionsMenu(*this, tools()->sc_road, scenario_tool_windows_.road);
	};
	scenario_toolmenu_.add(
	   /** TRANSLATORS: An entry in the editor's scenario tool menu */
	   _("Roads and Waterways"), ScenarioToolMenuEntry::kRoad,
	   g_gr->images().get("images/wui/editor/tools/sc_road.png"), false,
	   /** TRANSLATORS: Tooltip for the place roads scenario tool in the editor */
	   _("Build roads and waterways"));

	scenario_toolmenu_.add(
	   /** TRANSLATORS: An entry in the editor's scenario tool menu */
	   _("Settings"), ScenarioToolMenuEntry::kInfrastructureSettings,
	   g_gr->images().get("images/wui/editor/tools/sc_infra_settings.png"), false,
	   /** TRANSLATORS: Tooltip for the infrastructure settings scenario tool in the editor */
	   _("Create the initial settings for buildings, flags, workers, and ships"),
	   pgettext("hotkey", "Shift+I"));

	scenario_tool_windows_.lua.open_window = [this] {
		// TODO(Nordfriese): remove the warning when ScenarioLuaEditor is implemented properly
		UI::WLMessageBox m(this, _("Not yet implemented"),
		                   _("Please note that this feature is still under development and not in a "
		                     "useable state yet."),
		                   UI::WLMessageBox::MBoxType::kOkCancel);
		if (m.run<UI::Panel::Returncodes>() == UI::Panel::Returncodes::kOk)
			new ScenarioLuaEditor(*this, scenario_tool_windows_.lua);
	};
	/** TRANSLATORS: An entry in the editor's scenario tool menu */
	scenario_toolmenu_.add(_("Scripting"), ScenarioToolMenuEntry::kLua,
	                       g_gr->images().get("images/wui/editor/menus/scripting.png"), false,
	                       /** TRANSLATORS: Tooltip for the scenario scripting menu in the editor */
	                       _("Edit the scenario storyline"));
}

void EditorInteractive::scenario_tool_menu_selected(ScenarioToolMenuEntry entry) {
	switch (entry) {
	case ScenarioToolMenuEntry::kFinalize:
		finalize_clicked();
		break;
	case ScenarioToolMenuEntry::kFieldOwner:
		scenario_tool_windows_.fieldowner.toggle();
		break;
	case ScenarioToolMenuEntry::kVision:
		scenario_tool_windows_.vision.toggle();
		break;
	case ScenarioToolMenuEntry::kInfrastructure:
		scenario_tool_windows_.infrastructure.toggle();
		break;
	case ScenarioToolMenuEntry::kInfrastructureSettings:
		select_tool(tools()->sc_infra_settings, EditorTool::First);
		break;
	case ScenarioToolMenuEntry::kWorker:
		scenario_tool_windows_.worker.toggle();
		break;
	case ScenarioToolMenuEntry::kRoad:
		scenario_tool_windows_.road.toggle();
		break;
	case ScenarioToolMenuEntry::kLua:
		scenario_tool_windows_.lua.toggle();
		break;
	}
	scenario_toolmenu_.toggle();
}

void EditorInteractive::add_showhide_menu() {
	showhidemenu_.set_image(g_gr->images().get("images/wui/menus/showhide.png"));
	toolbar()->add(&showhidemenu_);

	rebuild_showhide_menu();

	showhidemenu_.selected.connect([this] { showhide_menu_selected(showhidemenu_.get_selected()); });
}

void EditorInteractive::rebuild_showhide_menu() {
	showhidemenu_.clear();

	/** TRANSLATORS: An entry in the editor's show/hide menu to toggle whether building spaces are
	 * shown */
	showhidemenu_.add(buildhelp() ? _("Hide Building Spaces") : _("Show Building Spaces"),
	                  ShowHideEntry::kBuildingSpaces,
	                  g_gr->images().get("images/wui/menus/toggle_buildhelp.png"), false, "",
	                  pgettext("hotkey", "Space"));

	/** TRANSLATORS: An entry in the editor's show/hide menu to toggle whether the map grid is shown
	 */
	showhidemenu_.add(draw_grid_ ? _("Hide Grid") : _("Show Grid"), ShowHideEntry::kGrid,
	                  g_gr->images().get("images/wui/menus/menu_toggle_grid.png"), false, "", "G");

	/** TRANSLATORS: An entry in the editor's show/hide menu to toggle whether immovables (trees,
	 * rocks etc.) are shown */
	showhidemenu_.add(draw_immovables_ ? _("Hide Immovables") : _("Show Immovables"),
	                  ShowHideEntry::kImmovables,
	                  g_gr->images().get("images/wui/menus/toggle_immovables.png"));

	/** TRANSLATORS: An entry in the editor's show/hide menu to toggle whether animals are shown */
	showhidemenu_.add(draw_bobs_ ? _("Hide Animals") : _("Show Animals"), ShowHideEntry::kAnimals,
	                  g_gr->images().get("images/wui/menus/toggle_bobs.png"));

	/** TRANSLATORS: An entry in the editor's show/hide menu to toggle whether resources are shown */
	showhidemenu_.add(draw_resources_ ? _("Hide Resources") : _("Show Resources"),
	                  ShowHideEntry::kResources,
	                  g_gr->images().get("images/wui/menus/toggle_resources.png"));

	if (finalized_)
		/** TRANSLATORS: An entry in the game's show/hide menu to toggle whether building names are
		 * shown */
		showhidemenu_.add(get_display_flag(dfShowCensus) ? _("Hide Census") : _("Show Census"),
		                  ShowHideEntry::kCensus,
		                  g_gr->images().get("images/wui/menus/toggle_census.png"), false, "", "C");
}

void EditorInteractive::showhide_menu_selected(ShowHideEntry entry) {
	switch (entry) {
	case ShowHideEntry::kBuildingSpaces: {
		toggle_buildhelp();
	} break;
	case ShowHideEntry::kGrid: {
		toggle_grid();
	} break;
	case ShowHideEntry::kImmovables: {
		toggle_immovables();
	} break;
	case ShowHideEntry::kAnimals: {
		toggle_bobs();
	} break;
	case ShowHideEntry::kResources: {
		toggle_resources();
	} break;
	case ShowHideEntry::kCensus: {
		set_display_flag(dfShowCensus, !get_display_flag(dfShowCensus));
	} break;
	}
	rebuild_showhide_menu();
}

void EditorInteractive::load(const std::string& filename) {
	assert(filename.size());

	Widelands::Map* map = egbase().mutable_map();

	cleanup_for_load();

	std::unique_ptr<Widelands::MapLoader> ml(map->get_correct_loader(filename));
	if (!ml.get())
		throw WLWarning(
		   _("Unsupported Format"),
		   _("Widelands could not load the file \"%s\". The file format seems to be incompatible."),
		   filename.c_str());
	ml->preload_map(true);

	UI::ProgressWindow* loader_ui = egbase().get_loader_ui();
	// We already have a loader window if Widelands was started with --editor=mapname
	const bool create_loader_ui = !loader_ui;
	if (create_loader_ui) {
		loader_ui = new UI::ProgressWindow("images/loadscreens/editor.jpg");
		GameTips editortips(*loader_ui, {"editor"});
		egbase().set_loader_ui(loader_ui);
	}

	load_all_tribes(&egbase());

	// Create the players. TODO(SirVer): this must be managed better
	// TODO(GunChleoc): Ugly - we only need this for the test suite right now
	loader_ui->step(_("Creating players"));
	iterate_player_numbers(p, map->get_nrplayers()) {
		if (!map->get_scenario_player_tribe(p).empty()) {
			egbase().add_player(
			   p, 0, map->get_scenario_player_tribe(p), map->get_scenario_player_name(p));
		}
	}

	ml->load_map_complete(egbase(), Widelands::MapLoader::LoadType::kEditor);
	egbase().postload();
	egbase().load_graphics();
	map_changed(MapWas::kReplaced);
	if (create_loader_ui) {
		// We created it, so we have to unset and delete it
		egbase().set_loader_ui(nullptr);
		delete loader_ui;
	}
}

void EditorInteractive::cleanup_for_load() {
	// TODO(unknown): get rid of cleanup_for_load, it tends to be very messy
	// Instead, delete and re-create the egbase.
	egbase().cleanup_for_load();
	unfinalize();
}

void EditorInteractive::unfinalize() {
	finalized_ = false;
	illustrating_vision_for_ = 0;
	allowed_buildings_windows_.clear();
	scripting_saver_.reset(nullptr);
	functions_.clear();
	variables_.clear();
	includes_.clear();
	set_display_flag(dfShowCensus, false);
	rebuild_scenario_tool_menu();
	rebuild_showhide_menu();
}

/// Called just before the editor starts, after postload, init and gfxload.
void EditorInteractive::start() {
	// Run the editor initialization script, if any
	try {
		g_sh->change_music("ingame", 1000);
		egbase().lua().run_script("map:scripting/editor_init.lua");
	} catch (LuaScriptNotExistingError&) {
		// do nothing.
	}
	map_changed(MapWas::kReplaced);
}

/**
 * Called every frame.
 *
 * Advance the timecounter and animate textures.
 */
void EditorInteractive::think() {
	InteractiveBase::think();

	uint32_t lasttime = realtime_;

	realtime_ = SDL_GetTicks();

	egbase().get_gametime_pointer() += realtime_ - lasttime;
}

void EditorInteractive::exit() {
	if (need_save_) {
		if (SDL_GetModState() & KMOD_CTRL) {
			end_modal<UI::Panel::Returncodes>(UI::Panel::Returncodes::kBack);
		} else {
			UI::WLMessageBox mmb(this, _("Unsaved Map"),
			                     _("The map has not been saved, do you really want to quit?"),
			                     UI::WLMessageBox::MBoxType::kOkCancel);
			if (mmb.run<UI::Panel::Returncodes>() == UI::Panel::Returncodes::kBack)
				return;
		}
	}
	g_sh->change_music("menu", 200);
	end_modal<UI::Panel::Returncodes>(UI::Panel::Returncodes::kBack);
}

void EditorInteractive::map_clicked(const Widelands::NodeAndTriangle<>& node_and_triangle,
                                    const bool should_draw) {
	history_->do_action(tools_->current(), tools_->use_tool, *egbase().mutable_map(),
	                    node_and_triangle, *this, should_draw);
	set_need_save(true);
}

bool EditorInteractive::handle_mouserelease(uint8_t btn, int32_t x, int32_t y) {
	if (btn == SDL_BUTTON_LEFT) {
		is_painting_ = false;
	}
	return InteractiveBase::handle_mouserelease(btn, x, y);
}

bool EditorInteractive::handle_mousepress(uint8_t btn, int32_t x, int32_t y) {
	if (btn == SDL_BUTTON_LEFT) {
		is_painting_ = true;
	}
	return InteractiveBase::handle_mousepress(btn, x, y);
}

static void draw_immovable_for_formerly_visible_field(const FieldsToDraw::Field& field,
                                                      const Widelands::Player::Field& player_field,
                                                      const float scale,
                                                      RenderTarget* dst) {
	if (player_field.map_object_descr == nullptr) {
		return;
	}

	if (player_field.constructionsite.becomes) {
		assert(field.owner != nullptr);
		player_field.constructionsite.draw(
		   field.rendertarget_pixel, field.fcoords, scale, field.owner->get_playercolor(), dst);

	} else if (upcast(const Widelands::BuildingDescr, building, player_field.map_object_descr)) {
		assert(field.owner != nullptr);
		// this is a building therefore we either draw unoccupied or idle animation
		dst->blit_animation(field.rendertarget_pixel, field.fcoords, scale,
		                    building->get_unoccupied_animation(), 0, &field.owner->get_playercolor());
	} else if (player_field.map_object_descr->type() == Widelands::MapObjectType::FLAG) {
		assert(field.owner != nullptr);
		dst->blit_animation(field.rendertarget_pixel, field.fcoords, scale,
		                    field.owner->tribe().flag_animation(), 0,
		                    &field.owner->get_playercolor());
	} else if (const uint32_t pic = player_field.map_object_descr->main_animation()) {
		dst->blit_animation(field.rendertarget_pixel, field.fcoords, scale, pic, 0,
		                    (field.owner == nullptr) ? nullptr : &field.owner->get_playercolor());
	}
}

void EditorInteractive::draw(RenderTarget& dst) {
	const auto& ebase = egbase();
	auto* fields_to_draw = map_view()->draw_terrain(ebase, Workareas(), draw_grid_, &dst);
	const auto& road_building = road_building_overlays();
	const auto& waterway_building = waterway_building_overlays();
	const auto info_to_draw = get_info_to_draw(!map_view()->is_animating());

	const float scale = 1.f / map_view()->view().zoom;
	const uint32_t gametime = ebase.get_gametime();

	// The map provides a mapping from player number to Coords, while we require
	// the inverse here. We construct this, but this is done on every frame and
	// therefore potentially expensive - though it never showed up in any of my
	// profiles. We could change the Map should this become a bottleneck, since
	// plrnum -> coords is needed less often.
	const auto& map = ebase.map();
	std::map<Widelands::Coords, int> starting_positions;
	for (int i = 1; i <= map.get_nrplayers(); ++i) {
		starting_positions[map.get_starting_pos(i)] = i;
	}

	// Figure out which fields are currently under the selection.
	std::set<Widelands::Coords> selected_nodes;
	std::set<Widelands::TCoords<>> selected_triangles;
	if (!get_sel_triangles()) {
		Widelands::MapRegion<> mr(map, Widelands::Area<>(get_sel_pos().node, get_sel_radius()));
		do {
			selected_nodes.emplace(mr.location());
		} while (mr.advance(map));
	} else {
		Widelands::MapTriangleRegion<> mr(
		   map, Widelands::Area<Widelands::TCoords<>>(get_sel_pos().triangle, get_sel_radius()));
		do {
			selected_triangles.emplace(mr.location());
		} while (mr.advance(map));
	}

	const auto& world = ebase.world();
	for (size_t idx = 0; idx < fields_to_draw->size(); ++idx) {
		FieldsToDraw::Field& field = *fields_to_draw->mutable_field(idx);

		Widelands::Player::Field const* const player_field =
		   illustrating_vision_for_ ?
		      &ebase.player(illustrating_vision_for_)
		          .fields()[map.get_index(field.fcoords, map.get_width())] :
		      nullptr;
		if (player_field) {
			field.vision = player_field->vision;
			if (field.vision <= 1) {
				field.road_e = field.vision ? player_field->r_e : Widelands::RoadSegment::kNone;
				field.road_se = field.vision ? player_field->r_se : Widelands::RoadSegment::kNone;
				field.road_sw = field.vision ? player_field->r_sw : Widelands::RoadSegment::kNone;
				field.owner = field.vision && player_field->owner != 0 ?
				                 ebase.get_player(player_field->owner) :
				                 nullptr;
				field.is_border = field.vision && player_field->border;
				// Allow the user a tiny sneak-peak at unseen fields for convenience
				field.brightness /= field.vision ? 2.f : 8.f;
			}
		}

		if (field.vision > 0) {
			const auto rinfo = road_building.road_previews.find(field.fcoords);
			if (rinfo != road_building.road_previews.end()) {
				for (uint8_t dir : rinfo->second) {
					switch (dir) {
					case Widelands::WALK_E:
						field.road_e = Widelands::RoadSegment::kNormal;
						break;
					case Widelands::WALK_SE:
						field.road_se = Widelands::RoadSegment::kNormal;
						break;
					case Widelands::WALK_SW:
						field.road_sw = Widelands::RoadSegment::kNormal;
						break;
					default:
						throw wexception(
						   "Attempt to set road-building overlay for invalid direction %i", dir);
					}
				}
			}
			const auto winfo = waterway_building.road_previews.find(field.fcoords);
			if (winfo != waterway_building.road_previews.end()) {
				for (uint8_t dir : winfo->second) {
					switch (dir) {
					case Widelands::WALK_E:
						field.road_e = Widelands::RoadSegment::kWaterway;
						break;
					case Widelands::WALK_SE:
						field.road_se = Widelands::RoadSegment::kWaterway;
						break;
					case Widelands::WALK_SW:
						field.road_sw = Widelands::RoadSegment::kWaterway;
						break;
					default:
						throw wexception(
						   "Attempt to set waterway-building overlay for invalid direction %i", dir);
					}
				}
			}

			draw_border_markers(field, scale, *fields_to_draw, &dst);

			if (draw_immovables_) {
				if (field.vision > 1) {
					Widelands::BaseImmovable* const imm = field.fcoords.field->get_immovable();
					if (imm != nullptr && imm->get_positions(ebase).front() == field.fcoords) {
						imm->draw(
						   gametime, info_to_draw, field.rendertarget_pixel, field.fcoords, scale, &dst);
					}
				} else if (field.vision > 0) {
					draw_immovable_for_formerly_visible_field(field, *player_field, scale, &dst);
				}
			}

			if (draw_bobs_ && field.vision > 1) {
				for (Widelands::Bob* bob = field.fcoords.field->get_first_bob(); bob;
				     bob = bob->get_next_bob()) {
					bob->draw(ebase, info_to_draw, field.rendertarget_pixel, field.fcoords, scale, &dst);
				}
			}

			// Draw resource overlay.
			uint8_t const amount = field.fcoords.field->get_resources_amount();
			if (draw_resources_ && amount > 0) {
				const std::string& immname =
				   world.get_resource(field.fcoords.field->get_resources())->editor_image(amount);
				if (!immname.empty()) {
					const auto* pic = g_gr->images().get(immname);
					blit_field_overlay(
					   &dst, field, pic, Vector2i(pic->width() / 2, pic->height() / 2), scale);
				}
			}

			// Draw build help.
			if (buildhelp()) {
				const auto* overlay = get_buildhelp_overlay(
				   tools_->current().nodecaps_for_buildhelp(field.fcoords, ebase));
				if (overlay != nullptr) {
					blit_field_overlay(&dst, field, overlay->pic, overlay->hotspot, scale);
				}
			}

			// Draw the player starting position overlays.
			const auto it = starting_positions.find(field.fcoords);
			if (it != starting_positions.end()) {
				const Image* player_image =
				   playercolor_image(it->second - 1, "images/players/player_position.png");
				assert(player_image != nullptr);
				constexpr int kStartingPosHotspotY = 55;
				blit_field_overlay(&dst, field, player_image,
				                   Vector2i(player_image->width() / 2, kStartingPosHotspotY), scale);
			}
		}

		// Draw selection markers on the field.
		if (selected_nodes.count(field.fcoords) > 0) {
			const Image* pic = get_sel_picture();
			blit_field_overlay(&dst, field, pic, Vector2i(pic->width() / 2, pic->height() / 2), scale);
		}

		// Draw selection markers on the triangles.
		if (field.all_neighbors_valid()) {
			const FieldsToDraw::Field& rn = fields_to_draw->at(field.rn_index);
			const FieldsToDraw::Field& brn = fields_to_draw->at(field.brn_index);
			const FieldsToDraw::Field& bln = fields_to_draw->at(field.bln_index);
			if (selected_triangles.count(
			       Widelands::TCoords<>(field.fcoords, Widelands::TriangleIndex::R))) {
				const Vector2i tripos(
				   (field.rendertarget_pixel.x + rn.rendertarget_pixel.x + brn.rendertarget_pixel.x) /
				      3,
				   (field.rendertarget_pixel.y + rn.rendertarget_pixel.y + brn.rendertarget_pixel.y) /
				      3);
				const Image* pic = get_sel_picture();
				blit_overlay(&dst, tripos, pic, Vector2i(pic->width() / 2, pic->height() / 2), scale);
			}
			if (selected_triangles.count(
			       Widelands::TCoords<>(field.fcoords, Widelands::TriangleIndex::D))) {
				const Vector2i tripos(
				   (field.rendertarget_pixel.x + bln.rendertarget_pixel.x + brn.rendertarget_pixel.x) /
				      3,
				   (field.rendertarget_pixel.y + bln.rendertarget_pixel.y + brn.rendertarget_pixel.y) /
				      3);
				const Image* pic = get_sel_picture();
				blit_overlay(&dst, tripos, pic, Vector2i(pic->width() / 2, pic->height() / 2), scale);
			}
		}

		if (field.vision > 0) {
			const auto itb = road_building.steepness_indicators.find(field.fcoords);
			if (itb != road_building.steepness_indicators.end()) {
				blit_field_overlay(&dst, field, itb->second,
				                   Vector2i(itb->second->width() / 2, itb->second->height() / 2),
				                   scale);
			}
			const auto itw = waterway_building.steepness_indicators.find(field.fcoords);
			if (itw != waterway_building.steepness_indicators.end()) {
				blit_field_overlay(&dst, field, itw->second,
				                   Vector2i(itw->second->width() / 2, itw->second->height() / 2),
				                   scale);
			}
		}
	}
}

/// Needed to get freehand painting tools (hold down mouse and move to edit).
void EditorInteractive::set_sel_pos(Widelands::NodeAndTriangle<> const sel) {
	bool const target_changed = tools_->current().operates_on_triangles() ?
	                               sel.triangle != get_sel_pos().triangle :
	                               sel.node != get_sel_pos().node;
	InteractiveBase::set_sel_pos(sel);
	if (target_changed && is_painting_) {
		map_clicked(sel, true);
	}
}

void EditorInteractive::set_sel_radius_and_update_menu(uint32_t const val) {
	if (tools_->current().has_size_one()) {
		set_sel_radius(0);
		return;
	}
	if (UI::UniqueWindow* const w = menu_windows_.toolsize.window) {
		dynamic_cast<EditorToolsizeMenu&>(*w).update(val);
	} else {
		set_sel_radius(val);
	}
}

void EditorInteractive::stop_painting() {
	is_painting_ = false;
}

bool EditorInteractive::player_hears_field(const Widelands::Coords&) const {
	return true;
}

void EditorInteractive::toggle_resources() {
	draw_resources_ = !draw_resources_;
}

void EditorInteractive::toggle_immovables() {
	draw_immovables_ = !draw_immovables_;
}

void EditorInteractive::toggle_bobs() {
	draw_bobs_ = !draw_bobs_;
}

void EditorInteractive::toggle_grid() {
	draw_grid_ = !draw_grid_;
}

bool EditorInteractive::handle_key(bool const down, SDL_Keysym const code) {
	if (down) {
		switch (code.sym) {
		// Sel radius
		case SDLK_1:
			if (code.mod & (KMOD_CTRL)) {
				toggle_buildhelp();
			} else {
				set_sel_radius_and_update_menu(0);
			}
			return true;
		case SDLK_2:
			if (code.mod & (KMOD_CTRL)) {
				toggle_immovables();
			} else {
				set_sel_radius_and_update_menu(1);
			}
			return true;
		case SDLK_3:
			if (code.mod & (KMOD_CTRL)) {
				toggle_bobs();
			} else {
				set_sel_radius_and_update_menu(2);
			}
			return true;
		case SDLK_4:
			if (code.mod & (KMOD_CTRL)) {
				toggle_resources();
			} else {
				set_sel_radius_and_update_menu(3);
			}
			return true;
		case SDLK_5:
			set_sel_radius_and_update_menu(4);
			return true;
		case SDLK_6:
			set_sel_radius_and_update_menu(5);
			return true;
		case SDLK_7:
			set_sel_radius_and_update_menu(6);
			return true;
		case SDLK_8:
			set_sel_radius_and_update_menu(7);
			return true;
		case SDLK_9:
			set_sel_radius_and_update_menu(8);
			return true;
		case SDLK_0:
			if (!(code.mod & KMOD_CTRL)) {
				set_sel_radius_and_update_menu(9);
				return true;
			}
			break;

		case SDLK_LSHIFT:
		case SDLK_RSHIFT:
			if (tools_->use_tool == EditorTool::First)
				select_tool(tools_->current(), EditorTool::Second);
			return true;

		case SDLK_LCTRL:
		case SDLK_RCTRL:
		// TODO(GunChleoc): Keeping ALT and MODE to make the transition easier. Remove for Build 20.
		case SDLK_LALT:
		case SDLK_RALT:
		case SDLK_MODE:
			if (tools_->use_tool == EditorTool::First)
				select_tool(tools_->current(), EditorTool::Third);
			return true;

		case SDLK_g:
			toggle_grid();
			return true;

		case SDLK_h:
			mainmenu_.toggle();
			return true;

		case SDLK_i:
			if (code.mod & KMOD_SHIFT) {
				if (finalized_) {
					select_tool(tools_->sc_infra_settings, EditorTool::First);
				}
			} else {
				select_tool(tools_->info, EditorTool::First);
			}
			return true;

		case SDLK_l:
			if (code.mod & (KMOD_LCTRL | KMOD_RCTRL)) {
				menu_windows_.loadmap.toggle();
			}
			return true;

		case SDLK_p:
			tool_windows_.players.toggle();
			return true;

		case SDLK_s:
			if (code.mod & (KMOD_LCTRL | KMOD_RCTRL)) {
				menu_windows_.savemap.toggle();
			} else {
				scenario_toolmenu_.toggle();
			}
			return true;

		case SDLK_t:
			toolmenu_.toggle();
			return true;

		case SDLK_c:
			if (finalized_) {
				set_display_flag(dfShowCensus, !get_display_flag(dfShowCensus));
				rebuild_showhide_menu();
			}
			return true;

		case SDLK_y:
			if (code.mod & (KMOD_LCTRL | KMOD_RCTRL))
				history_->redo_action();
			return true;

		case SDLK_z:
			if ((code.mod & (KMOD_LCTRL | KMOD_RCTRL)) && (code.mod & (KMOD_LSHIFT | KMOD_RSHIFT)))
				history_->redo_action();
			else if (code.mod & (KMOD_LCTRL | KMOD_RCTRL))
				history_->undo_action();
			return true;

		case SDLK_F1:
			menu_windows_.help.toggle();
			return true;
		case SDLK_ESCAPE:
			mainmenu_.toggle();
			return true;
		default:
			break;
		}
	} else {
		// key up events
		switch (code.sym) {
		case SDLK_LSHIFT:
		case SDLK_RSHIFT:
		case SDLK_LCTRL:
		case SDLK_RCTRL:
		// TODO(GunChleoc): Keeping ALT and MODE to make the transition easier. Remove for Build 20.
		case SDLK_LALT:
		case SDLK_RALT:
		case SDLK_MODE:
			if (tools_->use_tool != EditorTool::First)
				select_tool(tools_->current(), EditorTool::First);
			return true;
		default:
			break;
		}
	}
	return InteractiveBase::handle_key(down, code);
}

void EditorInteractive::select_tool(EditorTool& primary, EditorTool::ToolIndex const which) {
	if (&primary != tools_->current_pointer) {
		// Leave road building mode when unselecting the roads tool
		if (is_building_road())
			abort_build_road();
		if (is_building_waterway())
			abort_build_waterway();
		illustrating_vision_for_ =
		   &primary == &tools_->sc_vision ? tools_->sc_vision.get_player() : 0;
	}
	if (which == EditorTool::First && &primary != tools_->current_pointer) {
		if (primary.has_size_one()) {
			set_sel_radius(0);
			if (UI::UniqueWindow* const w = menu_windows_.toolsize.window) {
				EditorToolsizeMenu& toolsize_menu = dynamic_cast<EditorToolsizeMenu&>(*w);
				toolsize_menu.set_buttons_enabled(false);
			}
		} else {
			if (UI::UniqueWindow* const w = menu_windows_.toolsize.window) {
				EditorToolsizeMenu& toolsize_menu = dynamic_cast<EditorToolsizeMenu&>(*w);
				toolsize_menu.update(toolsize_menu.value());
			}
		}
		egbase().mutable_map()->recalc_whole_map(egbase());
	}
	tools_->current_pointer = &primary;
	tools_->use_tool = which;

	if (const Image* sel_pic = primary.get_sel(which)) {
		set_sel_picture(sel_pic);
	} else {
		unset_sel_picture();
	}
	set_sel_triangles(primary.operates_on_triangles());
}

void EditorInteractive::run_editor(const std::string& filename, const std::string& script_to_run) {
	Widelands::EditorGameBase egbase(nullptr);
	EditorInteractive& eia = *new EditorInteractive(egbase);
	egbase.set_ibase(&eia);  // TODO(unknown): get rid of this
	{
		UI::ProgressWindow loader_ui("images/loadscreens/editor.jpg");
		GameTips editortips(loader_ui, {"editor"});
		egbase.set_loader_ui(&loader_ui);

		{
			if (filename.empty()) {
				loader_ui.step(_("Creating empty map…"));
				egbase.mutable_map()->create_empty_map(
				   egbase, 64, 64, 0,
				   /** TRANSLATORS: Default name for new map */
				   _("No Name"),
				   get_config_string("realname",
				                     /** TRANSLATORS: Map author name when it hasn't been set yet */
				                     pgettext("author_name", "Unknown")));

				load_all_tribes(&egbase);

				egbase.load_graphics();
				loader_ui.step(std::string());
			} else {
				loader_ui.step((boost::format(_("Loading map “%s”…")) % filename).str());
				eia.load(filename);
			}
		}

		egbase.postload();
		egbase.set_loader_ui(nullptr);

		eia.start();

		if (!script_to_run.empty()) {
			eia.egbase().lua().run_script(script_to_run);
		}
	}
	eia.run<UI::Panel::Returncodes>();

	egbase.cleanup_objects();
}

void EditorInteractive::map_changed(const MapWas& action) {
	switch (action) {
	case MapWas::kReplaced:
		history_.reset(new EditorHistory(*undo_, *redo_));
		undo_->set_enabled(false);
		redo_->set_enabled(false);

		tools_.reset(new Tools(egbase().map()));
		select_tool(tools_->info, EditorTool::First);
		set_sel_radius(0);

		set_need_save(false);
		show_buildhelp(true);

		// Close all windows.
		for (Panel* child = get_first_child(); child; child = child->get_next_sibling()) {
			if (is_a(UI::Window, child)) {
				child->die();
			}
		}

		// Ensure that there is at least 1 player
		if (egbase().map().get_nrplayers() < 1) {
			Widelands::Map* mutable_map = egbase().mutable_map();
			mutable_map->set_nrplayers(1);
			// Init player 1
			mutable_map->set_scenario_player_ai(1, "");
			mutable_map->set_scenario_player_closeable(1, false);
			/** TRANSLATORS: Default player name, e.g. Player 1 */
			mutable_map->set_scenario_player_name(1, (boost::format(_("Player %u")) % 1).str());
			mutable_map->set_scenario_player_tribe(1, "");
			update_players();
		}

		rebuild_scenario_tool_menu();

		// Make sure that we will start at coordinates (0,0).
		map_view()->set_view(MapView::View{Vector2f::zero(), 1.f}, MapView::Transition::Jump);
		set_sel_pos(Widelands::NodeAndTriangle<>{
		   Widelands::Coords(0, 0),
		   Widelands::TCoords<>(Widelands::Coords(0, 0), Widelands::TriangleIndex::D)});
		break;

	case MapWas::kGloballyMutated:
		break;
	}
}

void EditorInteractive::update_players() {
	const Widelands::Map& map = egbase().map();
	Widelands::PlayersManager* pm = egbase().player_manager();
	assert(pm);
	pm->cleanup();
	const Widelands::PlayerNumber max = map.get_nrplayers();
	for (Widelands::PlayerNumber p = 1; p <= max; ++p) {
		pm->add_player(p, p, map.get_scenario_player_tribe(p), map.get_scenario_player_name(p));
	}
	egbase().allocate_player_maps();
}

EditorInteractive::Tools* EditorInteractive::tools() {
	return tools_.get();
}

void EditorInteractive::init_allowed_buildings_windows_registries() {
	assert(finalized_);
	assert(allowed_buildings_windows_.empty());
	const unsigned nrplayers = egbase().map().get_nrplayers();
	for (Widelands::PlayerNumber p = 1; p <= nrplayers; ++p) {
		UI::UniqueWindow::Registry* r = new UI::UniqueWindow::Registry();
		r->open_window = [this, p, r]() { new EditorPlayerAllowedBuildingsWindow(this, p, *r); };
		allowed_buildings_windows_.push_back(std::unique_ptr<UI::UniqueWindow::Registry>(r));
	}
}

void EditorInteractive::finalize_clicked() {
	assert(!finalized_);
	UI::WLMessageBox m(
	   this, _("Finalize"),
	   _("Are you sure you want to finalize this map?\n\n"
	     "This means you will not be able to add or remove players, rename them, "
	     "or change their tribe and starting position.\n"
	     "Nor can the waterway length limit be changed any more.\n\n"
	     "This step is only required if you want to design a scenario with the editor."),
	   UI::WLMessageBox::MBoxType::kOkCancel);
	if (m.run<UI::Panel::Returncodes>() != UI::Panel::Returncodes::kOk) {
		return;
	}
	const std::string result = try_finalize();
	if (result.empty()) {
		// Success!
		return;
	}
	UI::WLMessageBox error(this, _("Finalize Failed"),
	                       (boost::format(_("Finalizing failed! Reason: %s")) % result).str(),
	                       UI::WLMessageBox::MBoxType::kOk);
	error.run<UI::Panel::Returncodes>();
}

std::string EditorInteractive::try_finalize() {
	if (finalized_) {
		return _("Already finalized");
	}
	const Widelands::Map& map = egbase().map();
	const size_t nr_players = map.get_nrplayers();
	if (nr_players < 1) {
		return _("The map has no players");
	}
	const Widelands::Tribes& t = egbase().tribes();
	for (Widelands::PlayerNumber p = 1; p <= nr_players; ++p) {
		if (!t.tribe_exists(t.tribe_index(map.get_scenario_player_tribe(p)))) {
			return (boost::format(_("Invalid tribe \"%1$s\" for player %2$s (%3$s)")) %
			        map.get_scenario_player_tribe(p).c_str() % std::to_string(static_cast<int>(p)) %
			        map.get_scenario_player_name(p).c_str())
			   .str();
		}
		if (!map.get_starting_pos(p)) {
			return (boost::format(_("No starting position was set for player %s")) %
			        std::to_string(static_cast<int>(p)))
			   .str();
		}
	}
	finalized_ = true;
	init_allowed_buildings_windows_registries();
	new_scripting_saver();
	{  // Create a main function with an empty body
		LuaFunction* lf = new LuaFunction("mission_thread");
		lf->init(*scripting_saver_);
		FS_FunctionCall* fc = new FS_FunctionCall(lf, nullptr, {});
		fc->init(*scripting_saver_);
		FS_LaunchCoroutine* lc = new FS_LaunchCoroutine(fc);
		lc->init(*scripting_saver_);
		functions_.push_back(lc);
	}
	tool_windows_.players.destroy();
	menu_windows_.mapoptions.destroy();
	rebuild_scenario_tool_menu();
	rebuild_showhide_menu();
	select_tool(tools_->info, EditorTool::ToolIndex::First);
	return "";
}

void EditorInteractive::write_lua(FileWrite& fw) const {
	const Widelands::Map& map = egbase().map();

	// Header
	fw.print_f("-- %s\n-- %s\n\n",
	           /** TRANSLATORS: "build_version (build_config)", e.g. "build20 (Release)" */
	           (boost::format(_("Automatically created by Widelands %1$s (%2$s)")) %
	            build_id().c_str() % build_type().c_str())
	              .str()
	              .c_str(),
	           _("Do not modify this file. All changes will be discarded the next time this map is "
	             "saved in the editor."));

	// i18n
	{
		const char* mapname = map.get_name().c_str();
		std::string textdomain = "";
		for (const char* c = mapname; *c; ++c) {
			if (*c == ' ' || *c == '_' || *c == '-' || *c == '.') {
				textdomain += '_';
			} else if ((*c >= 'a' && *c <= 'z') || (*c >= '0' && *c <= '9')) {
				textdomain += *c;
			} else if (*c >= 'A' && *c <= 'Z') {
				textdomain += (*c + 'a' - 'A');
			}
		}
		if (textdomain.empty()) {
			log("WARNING: Map name '%s' unsuited for creating a set_textdomain() argument\n", mapname);
			textdomain = "invalid_name";
		}
		fw.print_f("set_textdomain(\"scenario_%s.wmf\")\n", textdomain.c_str());
	}

	// Builtin includes
	{
		// We always include these ones because they define run() and connected_road()
		std::set<std::string> builtin_includes = {"scripting/coroutine.lua"};
		// Check which other includes to include
		for (const FS_FunctionCall* f : scripting_saver_->all<FS_FunctionCall>()) {
			if (const BuiltinFunctionInfo* b = builtin_f(*f->get_function())) {
				if (!b->included_from.empty() && !builtin_includes.count(b->included_from)) {
					builtin_includes.insert(b->included_from);
				}
			}
		}
		fw.print_f("\n");
		for (const std::string& i : builtin_includes) {
			fw.print_f("include \"%s\"\n", i.c_str());
		}
	}

	// Global variables
	if (!variables_.empty()) {
		fw.print_f("\n");
		for (const auto& var : variables_) {
			var->write_lua(0, fw);
		}
	}

	// User-defined functions
	for (const auto& f : scripting_saver_->all<LuaFunction>()) {
		f->write_lua(0, fw);
		fw.print_f("\n");
	}

	// Hand-written includes
	// NOTE: Those should not contain "directly scripted" code but only functions which
	// can then be invoked from the user-defined functions here (not yet implemented)
	if (!includes_.empty()) {
		fw.print_f("\n");
		for (const std::string& i : includes_) {
			fw.print_f("include \"map:%s\"\n", i.c_str());
		}
	}

	// Autogenerate a function to initialize map objects and stuff as the editor user laboriously
	// defined them
	{
		std::string autogen_starting_function = "autogenerated";
		while (scripting_saver_->get<LuaFunction>(autogen_starting_function) ||
		       scripting_saver_->get<Variable>(autogen_starting_function)) {
			autogen_starting_function = "_" + autogen_starting_function;
		}
		fw.print_f("\nfunction %s()\n", autogen_starting_function.c_str());

		const char* var = nullptr;
		{
			std::string s = "temp_object";
			while (scripting_saver_->get<Variable>(s) || scripting_saver_->get<LuaFunction>(s)) {
				s = "_" + s;
			}
			var = s.c_str();
		}

#define write(text, ...) fw.print_f("   " text "\n", var, __VA_ARGS__)
		write("local %s");

		// Map objects

		std::set<const Widelands::Economy*> economies_to_save;
		{
			std::set<Widelands::Serial> saved_mos;
			const Widelands::Map& map = egbase.map();
			for (size_t map_index = map.max_index(); map_index; --map_index) {
				const Widelands::Coords coords = map.coords(map_index - 1);
				const Widelands::Field& f = map[map_index - 1];
				fw.unsigned_8(f.get_owned_by());
				if (f.get_immovable()) {
					bool skip = false;
					if (saved_mos.count(f.get_immovable()->serial())) {
						skip = true;
					} else {
						upcast(const Widelands::Building, bld, f.get_immovable());
						if (bld && bld->get_position() != coords) {
							skip = true;
						}
					}
					if (!skip) {
						saved_mos.insert(f.get_immovable()->serial());
						const Widelands::MapObjectType type = f.get_immovable()->descr().type();
						fw.unsigned_8(static_cast<uint8_t>(type));
						switch (type) {
						case Widelands::MapObjectType::FLAG: {
							const Widelands::Flag& flag =
							   dynamic_cast<const Widelands::Flag&>(*f.get_immovable());
							if (!economies_to_save.count(flag.get_economy(Widelands::wwWARE)))
								economies_to_save.insert(flag.get_economy(Widelands::wwWARE));
							if (!economies_to_save.count(flag.get_economy(Widelands::wwWORKER)))
								economies_to_save.insert(flag.get_economy(Widelands::wwWORKER));
							write("%s = wl.Game().players[%u]:place_flag(wl.Game().map:get_field(%u, %u), "
							      "true)",
							      flag.owner().player_number(), coords.x, coords.y);
							if (flag.current_wares()) {
								std::map<std::string, unsigned> wares;
								for (const WareInstance* w : flag.get_wares()) {
									auto it = wares.find(w->descr().name());
									if (it == wares.end())
										wares[w->descr().name()] = 1;
									else
										++it->second;
								}
								fw.print_f("   %s:set_wares({\n", var);
								for (const auto& pair : wares)
									fw.print_f("      %s = %u,\n", pair.first.c_str(), pair.second);
								fw.print_f("   })\n");
							}
						} break;
						case Widelands::MapObjectType::ROAD:
						case Widelands::MapObjectType::WATERWAY: {
							const Widelands::RoadBase& road =
							   dynamic_cast<const Widelands::RoadBase&>(*f.get_immovable());
							const Widelands::Road* r = dynamic_cast<const Widelands::Road*>(&road);
							fw.print_f("   %s = wl.Game().players[%u]:place_road(%s, "
							           "wl.Game().map:get_field(%u, %u).immovable",
							           var, road.owner().player_number(),
							           r ? r->is_busy() ? "busy" : "normal" : "waterway",
							           road.get_flag(Widelands::RoadBase::FlagStart).get_position().x,
							           road.get_flag(Widelands::RoadBase::FlagStart).get_position().y);
							const size_t n = road.get_path().get_nsteps();
							for (size_t i = 0; i < n; ++i) {
								const char* dir = nullptr;
								switch (road.get_path()[i]) {
								case Widelands::WALK_E:
									dir = "e";
									break;
								case Widelands::WALK_NE:
									dir = "ne";
									break;
								case Widelands::WALK_SE:
									dir = "se";
									break;
								case Widelands::WALK_W:
									dir = "w";
									break;
								case Widelands::WALK_NW:
									dir = "nw";
									break;
								case Widelands::WALK_SW:
									dir = "sw";
									break;
								default:
									NEVER_HERE()
								}
								fw.print_f(", %s", dir);
							}
							fw.print_f(")\n");
							if (r) {
								std::map<std::string, unsigned> workers;
								for (const Widelands::Carrier* c : r->carriers()) {
									auto it = workers.find(c->descr().name());
									if (it == workers.end())
										workers[c->descr().name()] = 1;
									else
										++it->second;
								}
								fw.print_f("   %s:set_workers({\n", var);
								for (const auto& pair : workers)
									fw.print_f("      %s = %u,\n", pair.first.c_str(), pair.second);
								fw.print_f("   })\n");
							} else {
								if (const Widelands::Ferry* ferry =
								       dynamic_cast<Widelands::Waterway&>(road).get_ferry()) {
									write("%s:set_workers(\"%s\", 1)", ferry->descr().name().c_str());
								}
							}
						} break;

#nocom buildings of all types
						}
					}
				}
			}
		}

		for (const Widelands::Economy* e : economies_to_save) {
			const Widelands::Coords flag = e->get_arbitrary_flag()->get_position();
			if (e->type() == Widelands::wwWARE) {
				write("%s = wl.Game().map:get_field(%u, %u).immovable.ware_economy", flag.x, flag.y);
				for (Widelands::DescriptionIndex di : e->owner().tribe().wares()) {
					write("%s:set_target_quantity(%s, %u)",
					      egbase.tribes().get_ware_descr(di)->name().c_str(),
					      e->target_quantity(di).permanent);
				}
			} else {
				write("%s = wl.Game().map:get_field(%u, %u).immovable.worker_economy", flag.x, flag.y);
				for (Widelands::DescriptionIndex di : e->owner().tribe().workers()) {
					write("%s:set_target_quantity(%s, %u)",
					      egbase.tribes().get_worker_descr(di)->name().c_str(),
					      e->target_quantity(di).permanent);
				}
			}
		}

		// Players: Teams, relations, territory, vision

		const uint8_t nrplayers = map.get_nrplayers();
		for (unsigned p = 1; p <= nrplayers; ++p) {
			write("%s = wl.Game().players[%u]", p);
			const Widelands::Player& player = egbase.player(p);
			write("%s.team = %u", player.team_number());
			for (unsigned p2 = 1; p2 <= nrplayers; ++p2) {
				if (p2 != p) {
					write("%s:set_attack_forbidden(%u, %s)", p2,
					      player.is_attack_forbidden(p2) ? "true" : "false");
				}
			}
			for (size_t map_index = map.max_index(); map_index; --map_index) {
				const Widelands::Coords c = map.coords(map_index - 1);
				if (map[map_index - 1].get_owned_by() == p) {
					write("%s:conquer(wl.Game().map:get_field(%u, %u), 0)", c.x, c.y);
				}
				const int vis = player.vision(map_index - 1);
				if (vis > 0) {
					write("%s:reveal_fields({wl.Game().map:get_field(%u, %u)})", c.x, c.y);
					if (vis < 2) {
						write("%s:hide_fields({wl.Game().map:get_field(%u, %u)})", c.x, c.y);
					}
				}
			}
		}

#undef write
		// Immediately invoke the function (not as a coroutine, to enforce immediate initialization)
		fw.print_f("end\n%s()\n", autogen_starting_function.c_str());
	}

	// Main function(s) call
	assert(!functions_.empty());
	for (const auto& f : functions_) {
		fw.print_f("\n");
		f->write_lua(0, fw);
	}
}
