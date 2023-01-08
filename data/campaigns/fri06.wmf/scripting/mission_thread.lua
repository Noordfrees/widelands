nr_danish_ships_received = 0

function launch_ships()
   local sleeptime = {1000 * 60 * 15, 1000 * 60 * 5, 1000 * 60}
   sleeptime = sleeptime[difficulty]

   local count = 0
   while true do
      -- Send a ship sailing around. To make sure the ship goes exactly where we want it, load a dummy ware and route it to the end port.
      count = count + 1
      local ship = p2:place_ship(ship_insert_field)
      ship.shipname = _("Ship %d"):bformat(count)

      starting_port:set_wares("log", starting_port:get_wares("log") + 1)
      starting_port:set_warehouse_policies("all", "remove")

      sleep(sleeptime)
   end
end

function receive_ships()
   local fields = {
      map:get_field(6, 8),
      map:get_field(6, 7),
      map:get_field(7, 6),
      map:get_field(7, 5),
      map:get_field(8, 4),
      map:get_field(7, 4),
      map:get_field(5, 9),
      map:get_field(5, 10),
      map:get_field(4, 11),
      map:get_field(4, 9),
      map:get_field(4, 10),
      map:get_field(3, 9),
      map:get_field(3, 10),
   }

   while true do
      sleep(500)
      for _, f in ipairs(fields) do
         for _,bob in pairs(f.bobs) do
            if bob.descr.name == "empire_ship" then
               nr_danish_ships_received = nr_danish_ships_received + 1
               bob:remove()
            end
         end
      end
   end
end

function spot_enemy_ship()
   while true do
      sleep(500)
      for _,ship in ipairs(p2:get_ships()) do
         if p1:sees_field(ship.field) then
            local prior_center = scroll_to_field(ship.field)
            sleep(1000)
            campaign_message_box(enemy_ship_spotted)
            scroll_to_map_pixel(prior_center)
            return
         end
      end
   end
end

function mission_thread()
   local perm_hidden_fringe = 10
   for x = -perm_hidden_fringe, perm_hidden_fringe do
      for y = 0, map.height - 1 do
         p1:hide_fields({map:wrap_field(x, y)}, "permanent")
      end
   end
   for y = -perm_hidden_fringe, perm_hidden_fringe do
      for x = 0, map.width - 1 do
         p1:hide_fields({map:wrap_field(x, y}, "permanent")
      end
   end

   campaign_message_box(intro_1)
   campaign_message_box(intro_2)
   campaign_message_box(intro_3)

   include "map:scripting/starting_conditions.lua"
   run(receive_ships)
   run(launch_ships)
   run(spot_enemy_ship)

   sleep(2000)
   campaign_message_box(intro_4)
   o_explore = add_campaign_objective(obj_explore_and_land)

   while #p1:get_buildings("frisians_port") == 0 do sleep(500) end

   local first_port = p1:get_buildings("frisians_port")[1]
   first_port:set_soldiers({[harke] = 1, [eschel] = 1})
   scroll_to_field(first_port.fields[1])

   sleep(1000)
   campaign_message_box(first_port_built_1)
   local o_wood = add_campaign_objective(obj_wood)
   sleep(1000)
   campaign_message_box(first_port_built_2)
   add_campaign_objective(obj_soldiers)

   run(function()
      while #p1:get_buildings("frisians_port") < 2 do sleep(2000) end
      set_objective_done(o_explore)
      campaign_message_box(second_port_built)
   end)

   while not check_for_buildings(p1, {
      frisians_foresters_house = 4,
      frisians_woodcutters_house = 4,
      frisians_reed_farm = 1,
      frisians_brick_kiln = 1,
      frisians_clay_pit = 1,
      frisians_well = 1,
   }) do sleep(1000) end
   set_objective_done(o_wood)

   local o_block = add_campaign_objective(obj_block)
   while starting_port.fleet == end_port.fleet do sleep(8000) end
   set_objective_done(o_block)

   local data = {
      harke = false,
      eschel = false,
      ships = nr_danish_ships_received
   }
   local cheat = false
   for x = 0, map.width - 1 do
      for y = 0, map.height - 1 do
         for i,bob in pairs(map:get_field(x, y).bobs) do
            if bob.descr.name == "frisians_soldier" then
               if bob.health_level == eschel[1] and bob.attack_level == eschel[2] and bob.defense_level == eschel[3] then
                  if data.eschel then cheat = true else data.eschel = true end
               elseif bob.health_level == harke[1] and bob.attack_level == harke[2] and bob.defense_level == harke[3] then
                  if data.harke then cheat = true else data.harke = true end
               else
                  cheat = true
               end
            end
         end
      end
   end
   if cheat then
      data.harke = false
      data.eschel = false
      campaign_message_box(cheated)
   end
   game:save_campaign_data("frisians", "fri06", data)
   p1:mark_scenario_as_solved("fri06.wmf")

   campaign_message_box(victory_1)
   -- END OF MISSION 6
end

run(mission_thread)
