dirname = path.dirname(__file__)

tribes:new_ware_type {
   msgctxt = "ware",
   name = "cannonball",
   -- TRANSLATORS: This is a ware name used in lists of wares
   descname = pgettext("ware", "Cannon Ball"),
   helptext_script = dirname .. "helptexts.lua",
   icon = dirname .. "menu.png",
   default_target_quantity = {
      atlanteans = 20,
      barbarians = 20,
      frisians = 20,
      empire = 20
   },
   preciousness = {
      atlanteans = 10,
      barbarians = 10,
      frisians = 10,
      empire = 10
   },

   animations = {
      idle = {
         pictures = path.list_files(dirname .. "idle.png"),
         hotspot = { 5, 5 },
      },
   }
}
