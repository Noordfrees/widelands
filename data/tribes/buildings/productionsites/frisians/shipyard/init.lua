push_textdomain("tribes")

local dirname = path.dirname (__file__)

wl.Descriptions():new_productionsite_type {
   name = "frisians_shipyard",
   -- TRANSLATORS: This is a building name used in lists of buildings
   descname = pgettext ("frisians_building", "Shipyard"),
   animation_directory = dirname,
   icon = dirname .. "menu.png",
   size = "medium",
   map_check = {"seafaring"},

   buildcost = {
      brick = 3,
      granite = 1,
      log = 3,
      reed = 2,
      cloth = 1
   },
   return_on_dismantle = {
      brick = 2,
      log = 2,
      reed = 1
   },

   spritesheets = {
      idle = {
         hotspot = {50, 78},
         frames = 10,
         columns = 5,
         rows = 2,
         fps = 10
      }
   },
   animations = {
      unoccupied = {
         hotspot = {50, 58}
      }
   },

   aihints = {
      needs_water = true,
      shipyard = true,
      prohibited_till = 1050
   },

   working_positions = {
      frisians_shipwright = 1
   },

   inputs = {
      { name = "log", amount = 10 },
      { name = "cloth", amount = 6 },
      { name = "clay", amount = 3 }
   },

   programs = {
      main = {
         -- TRANSLATORS: Completed/Skipped/Did not start working because ...
         descname = _("working"),
         actions = {
            -- steps from building to ship: 2-9
            -- worker time: 5.5 sec
            -- number of wares to carry: 10 + 2 + 6 = 18
            -- (see data/tribes/immovables/shipconstruction_frisians/init.lua)
            -- min. time total: 18 * (2 * 2 * 1.8 + 5.5 + 20 + 35) = 1218.6 sec
            -- max. time total: 18 * (2 * 9 * 1.8 + 5.5 + 20 + 35) = 1672.2 sec
            "call=ship on failure fail",
            "call=ship_preparation",
         }
      },
      ship = {
         -- TRANSLATORS: Completed/Skipped/Did not start constructing a ship because ...
         descname = _("constructing a ship"),
         actions = {
            "construct=frisians_shipconstruction worker:buildship radius:6",
            "sleep=duration:20s",
         }
      },
      ship_preparation = {
         descname = _("working"),
         actions = {
            "sleep=duration:35s",
            -- no working animation yet
         }
      },
   },
}

pop_textdomain()
