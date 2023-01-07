-- =========================
-- Some formating functions
-- =========================

include "scripting/richtext_scenarios.lua"

-- TODO(Nordfriese): Need new images for them all
function claus(title, text)
   return speech ("map:claus.png", "1CC200", title, text)
end
function henneke(title, text)
   return speech ("map:henneke.png", "F7FF00", title, text)
end
function reebaud(title, text)
   return speech ("map:reebaud.png", "55BB55", title, text)
end

-- =======================================================================
--                           Texts - No logic here
-- =======================================================================

local objective_message_height = 550

-- ===========
-- objectives
-- ===========

obj_explore_and_land = {
   name = "explore_and_land",
   title =_("Explore"),
   number = 1,
   body = objective_text(_("Explore the Kattegat."),
      li(_([[Explore the area with your expedition ships.]])) ..
      li(_([[When you have seen enough, build at least one port.]]))
   ),
}

obj_wood = {
   name = "wood",
   title =_("Wood economy"),
   number = 1,
   body = objective_text(_("Secure Your Wood Production."),
      li(_([[Build at least five woodcutter’s houses.]])) ..
      li(_([[Build at least five forester’s houses.]])) ..
      li(_([[Build a building materials supply chain.]]))
   ),
}

obj_block = {
   name = "block",
   title =_("Block the Kattegat"),
   number = 1,
   body = objective_text(_("Block the Sea Straits."),
      li(_([[Render the sea strait impassable by building dikes across its entire width from West to East.]]))
   ),
}

-- ==================
-- Texts to the user
-- ==================

intro_1 = {
   title = _("Introduction"),
   body=claus(_("Welcome back!"),
      -- TRANSLATORS: Claus Lembeck – Introduction 1
      _([[NOCOM.]])
      .. paragraphdivider() ..
      -- TRANSLATORS: Claus Lembeck – Introduction 1
      _([[NOCOM.]]))
}





victory_1 = {
   title = _("Victory"),
   body=reebaud(_("Victory!"),
      -- TRANSLATORS: Reebaud – Victory 1
      _([[NOCOM.]]))
      .. objective_text(_("Congratulations"),
      _([[You have completed this mission. You may continue playing if you wish, otherwise move on to the next mission.]])),
   allow_next_scenario = true,
}

cheated = {
      title = _("Cheated"),
      body = p(_([[You appear to have cheated by recruiting or training soldiers, instead of using Ocke Ketelsen’s sons as you were supposed to. This breach of faith will not reflect at all well on you upon your return to Helgoland.]])),
      w = 450,
      h = 150,
}
