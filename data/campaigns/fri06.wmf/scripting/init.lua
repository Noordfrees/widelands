-- =======================================================================
--                         Frisian Campaign Mission 6
-- =======================================================================
push_textdomain("scenario_fri06.wmf")

include "scripting/coroutine.lua"
include "scripting/field_animations.lua"
include "scripting/objective_utils.lua"
include "scripting/infrastructure.lua"
include "scripting/table.lua"
include "scripting/ui.lua"
include "scripting/messages.lua"

game = wl.Game()
map = game.map
p1 = game.players[1]
p2 = game.players[2]
difficulty = wl.Game().scenario_difficulty
wl.Game().allow_diplomacy = false

portfield_start = {245, 254}
portfield_end = {5, 6}
ship_insert_field = map:get_field(242, 255)

harke  = {{2, 4, 2, 0}, {2, 1, 2, 0}, {1, 0, 1, 0}}
eschel = {{1, 6, 1, 0}, {1, 3, 1, 0}, {0, 2, 0, 0}}
harke  = harke[difficulty]
eschel = eschel[difficulty]

include "map:scripting/texts.lua"
include "map:scripting/mission_thread.lua"
