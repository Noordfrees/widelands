-- =======================================================================
--                                 Player 1
-- =======================================================================
p1:allow_buildings("all")
p1:forbid_buildings({
   "frisians_barracks",
   "frisians_training_camp",
   "frisians_training_arena",
   "frisians_armor_smithy_small",
   "frisians_armor_smithy_large",
   "frisians_sewing_room",
   "frisians_tailors_shop",
   "frisians_recycling_center",
})

local function launch_expedition(x, y, items)
   local buildcost = 1
   local port = game:get_building_description(game:get_tribe_description(p1.tribe_name).port)
   for s,i in pairs(port.buildcost) do buildcost = buildcost + i end

   local ship = p1:place_ship(map:get_field(x, y))

   local nr_i = 0
   for s,i in pairs(items) do nr_i = nr_i + i end
   ship.capacity = buildcost + nr_i

   ship:make_expedition(items)
end

local flagship_field = {79, 24}
launch_expedition(flagship_field[1], flagship_field[2], {
         granite = 1,
         brick = 3,
         reed = 5,
         log = 4,
         frisians_brickmaker = 1,
         frisians_reed_farmer = 1,
         frisians_forester = 2,
      })
launch_expedition(83, 21, {
         granite = 1,
         brick = 3,
         reed = 3,
         log = 2,
         frisians_stonemason = 2,
         frisians_woodcutter = 3,
         frisians_forester = 3,
      })
launch_expedition(90, 23, {
         iron = 2,
         reed = 2,
         brick = 3,
         frisians_claydigger = 1,
         frisians_geologist = 1,
         frisians_miner = 2,
         frisians_smelter = 2,
         frisians_blacksmith = 1,
         frisians_smoker = 1,
         frisians_fisher = 1,
         frisians_woodcutter = 2,
      })

if difficulty < 3 then
   launch_expedition(96, 18, {
         log = 2,
         brick = 3,
         reed = 2,
         granite = 3,
         iron = 3,
         coal = 2,
      })
end
if difficulty < 2 then
launch_expedition(69, 22, {
         log = 3,
         brick = 2,
         reed = 3,
         granite = 2,
         iron = 2,
         coal = 3,
      })
end
scroll_to_field(map:get_field(flagship_field[1], flagship_field[2]))

-- =======================================================================
--                                 Player 2
-- =======================================================================
-- This player mustn't really build up an economy, most buildings are only allowed to look nice.
p2:allow_buildings("all")
p2:forbid_buildings({
   "empire_port",
   "empire_shipyard",
   "empire_sentry",
   "empire_blockhouse",
   "empire_outpost",
   "empire_barrier",
   "empire_tower",
   "empire_fortress",
   "empire_castle",
   "empire_trainingcamp",
   "empire_arena",
   "empire_colosseum",
   "empire_barracks",
   "empire_armorsmithy",
   "empire_weaponsmithy",
})

local soldiers = {
   {{0, 0, 0, 0}},
   {{0, 0, 0, 0}, {1, 0, 0, 0}, {0, 0, 0, 1}},
   {{1, 0, 0, 0}, {0, 0, 0, 1}, {1, 0, 0, 1}},
}
soldiers = soldiers[difficulty]
local function place(x, y, building, name)
   local b = p2:place_building(building or "empire_sentry", map:get_field(x, y), false, true)
   b.destruction_blocked = true
   if b.descr.type_name == "militarysite" then b:set_soldiers(soldiers[math.random(#soldiers)], 1) end
   if building == "empire_warehouse" then
      b:set_wares({
         log = 40,
         granite = 40,
         planks = 50,
         marble = 30,
         marble_column = 20,
         iron = 30,
         coal = 30,
         cloth = 20,
         water = 10,
         flour = 10,
         wheat = 10,
      })
      b:set_workers({
         empire_builder = 10,
         empire_lumberjack = 8,
         empire_forester = 8,
         empire_stonemason = 4,
         empire_toolsmith = 3,
         empire_carpenter = 3,
         empire_donkey = 5,
         empire_donkeybreeder = 1,
         empire_weaver = 3,
         empire_fisher = 5,
         empire_shepherd = 2,
         empire_farmer = 9,
         empire_miller = 4,
         empire_baker = 4,
         empire_brewer = 4,
         empire_pigbreeder = 3,
         empire_charcoal_burner = 2,
         empire_smelter = 4,
      })
   end
   if name ~= nil then b.warehousename = name end
   return b
end

-- TRANSLATORS: This Danish port is named after the Capital of Denmark
starting_port = place(portfield_start[1], portfield_start[2], "empire_port", _("København"))
end_port = place(portfield_end[1], portfield_end[2], "empire_port", _("Northern Frisia"))

place(89, 177,  "empire_outpost")
place(100, 212)
place(156, 205, "empire_tower")
place(158, 206, "empire_warehouse")
place(189, 203)
place(217, 205)
place(180, 138, "empire_outpost")
place(182, 140, "empire_warehouse")
place(143, 147)
place(70, 152)
place(69, 154, "empire_warehouse")
place(98, 143)
place(96, 140, "empire_warehouse")
place(137, 113, "empire_barrier")
place(90, 80)
place(154, 68, "empire_barrier")
place(154, 63, "empire_warehouse")
place(142, 34)
place(146, 35)
place(143, 33, "empire_warehouse")
place(102, 48)
place(101, 45)
place(105, 49, "empire_warehouse")
