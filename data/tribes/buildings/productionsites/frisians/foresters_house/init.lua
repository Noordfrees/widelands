push_textdomain("tribes")

local dirname = path.dirname(__file__)

wl.Descriptions():new_productionsite_type {
   name = "frisians_foresters_house",
   -- TRANSLATORS: This is a building name used in lists of buildings
   descname = pgettext("frisians_building", "Forester’s House"),
   animation_directory = dirname,
   icon = dirname .. "menu.png",
   size = "small",

   buildcost = {
      brick = 1,
      log = 1,
      reed = 1
   },
   return_on_dismantle = {
      brick = 1,
      log = 1,
   },

   spritesheets = {
      idle = {
         hotspot = {40, 73},
         frames = 10,
         columns = 5,
         rows = 2,
         fps = 10
      }
   },
   animations = {
      unoccupied = {
         hotspot = {40, 56}
      }
   },

   aihints = {
      space_consumer = true,
   },

   working_positions = {
      frisians_forester = 1
   },

   programs = {
      main = {
         -- TRANSLATORS: Completed/Skipped/Did not start planting trees because ...
         descname = _("planting trees"),
         actions = {
            -- time of worker: 12.2-33.8 sec, mean 22.784 sec
            -- min. time total: 12.2 + 12 = 24.2 sec
            -- max. time total: 33.8 + 12 = 45.8 sec
            -- mean time total: 22.784 + 12 = 34.784 sec
            "callworker=plant",
            "sleep=duration:12s"
         }
      },
   },
}

pop_textdomain()
