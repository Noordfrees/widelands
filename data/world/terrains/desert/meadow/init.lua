push_textdomain("world")

wl.World():new_terrain_type{
   name = "meadow",
   descname = _ "Meadow",
   is = "arable",
   tooltips = {
      -- TRANSLATORS: This is an entry in a terrain tooltip. Try to use 1 word if possible.
      _"likes trees",
   },
   valid_resources = {"resource_water"},
   default_resource = "resource_water",
   default_resource_amount = 10,
   textures = { path.dirname(__file__) .. "idle.png" },
   dither_layer = 310,
   temperature = 160,
   humidity = 600,
   fertility = 600,
}

pop_textdomain()
