-- =========================
-- Some formating functions
-- =========================

include "scripting/richtext_scenarios.lua"

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
      li(_([[When you have seen enough, build at least two ports.]]))
   ),
}

obj_soldiers = {
   name = "soldiers",
   title =_("Protect your soldiers"),
   number = 1,
   body = objective_text(_("Employ Harke and Eschel Ockesen Wisely."),
      li(_([[Your only soldiers are Ocke Ketelsen’s two sons. Try to keep them alive, or their father will be angry.]]))
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
      _([[It has begun at last. It has been half a year since Waldemar Atterdag, King of the Danes, drove me out of my home, Föhr, and forced me to flee farther than I had ever imagined I might travel. And now our powerful alliance is starting to fight back.]]))
}
intro_2 = {
   title = _("Introduction"),
   body=claus(_("Welcome back!"),
      -- TRANSLATORS: Claus Lembeck – Introduction 2
      _([[We set sail from Helgoland in Midwinter. It is a bad time for warfare, but Atterdag in his exposed Northern empire will suffer more from winter than we with our interim base on the more temperate Helgoland. The storms that imperil sailors in late autumn have subsided, and drift ice is rare in these parts. The only danger here are the Danish warships, but they are slower than our vessels, and evading the ones that spotted us was not hard.]])
      .. paragraphdivider() ..
      -- TRANSLATORS: Claus Lembeck – Introduction 2
      _([[We have arrived in the Kattegat. To our surprise, none of our sea maps cover this area. We know that we are at the north-western end of the sea strait, which leads to the south-east towards the Danish capital. The region is a few miles wide and dotted with small islands. We believe they are but lightly guarded, if at all.]]))
}
intro_3 = {
   title = _("Introduction"),
   body=claus(_("Welcome back!"),
      -- TRANSLATORS: Claus Lembeck – Introduction 3
      _([[This should be easy. We will land on some of these islands and build up a basic economy, with especial emphasis on growing lots of wood. We will construct dikes to build a solid wall that connects the western and eastern shores of the Kattegat so that no ship will be able to sail from the Danish capital to the North Sea. Once that is accomplished, we will return to Helgoland and initiate the invasion proper.]]))
}
intro_4 = {
   title = _("Introduction"),
   body=reebaud(_("Welcome back!"),
      -- TRANSLATORS: Reebaud – Introduction 4
      _([[There is no need to hurry now. Before we land, we should use our ships to navigate the Kattegat and get a more detailed idea about the layout of the islands and any Danish strongholds there might be.]])
      .. paragraphdivider() ..
      -- TRANSLATORS: Reebaud – Introduction 4
      _([[We should also decide how many ports we will build. On the one hand, building fewer ports will leave us with leftover construction supplies which will make everything so much easier; on the other hand, building more ports will give us more space to build our economy and more starting points for diking. We should have at least two ports, and once we know the lay of the land we can decide how many more we will need.]]))
      .. new_objectives(obj_explore_and_land),
   h = objective_message_height,
}

first_port_built_1 = {
   title = _("First Port Complete"),
   body=henneke(_("Let’s begin"),
      -- TRANSLATORS: Henneke Lembeck – First Port Built 1
      _([[Our first port is complete. We can now begin to build our economy.]])
      .. paragraphdivider() ..
      -- TRANSLATORS: Henneke Lembeck – First Port Built 1
      _([[We will need all the building materials, of course, but our main focus must be wood. We will need masses of it to build dikes. Let us build plenty of woodcutter’s and forester’s houses, and later we may need to mine iron to forge additional shovels and axes.]]))
      .. new_objectives(obj_wood),
   position = "topleft",
   h = objective_message_height,
}
first_port_built_2 = {
   title = _("First Port Complete"),
   body=reebaud(_("Let’s begin"),
      -- TRANSLATORS: Reebaud – First Port Built 2
      _([[One thing to keep in mind is that we do not have many soldiers. Anticipating hardly any fighting, we decided to take no soldier other than Ocke Ketelsen’s two sons. They have received some battle training and will be able to deal with any enemy resistance by themselves.]])
      .. paragraphdivider() ..
      -- TRANSLATORS: Reebaud – First Port Built 2
      _([[Since we do not expect the enemies to fight back, we can dismantle our military buildings as soon as they are occupied. We do not have the soldiers to man them permanently. Obviously, we must be careful not to send our boys into situations beyond their ability to handle.]]))
      .. new_objectives(obj_soldiers),
   position = "topleft",
   h = objective_message_height,
}

second_port_built = {
   title = _("Second Port Complete"),
   body=henneke(_("Let’s begin"),
      -- TRANSLATORS: Henneke Lembeck – Second Port Built
      _([[We now have two ports. Now we should have plenty of space to build our economy.]]))
}

enemy_ship_spotted = {
   title = _("Enemy Ship Spotted"),
   body=claus(_("Watching the enemy"),
      -- TRANSLATORS: Claus Lembeck – Enemy Ship Spotted
      _([[We have spotted a Danish ship that appears to be travelling from the Danish capital towards the North Sea. They seem to pass though here in regular intervals. Let us block the straits quickly – every ship that is allowed to pass brings more troops to Föhr and the surrounding islands we will have to face in battle later.]])),
   position = "topleft",
}

victory_1 = {
   title = _("Victory"),
   body=claus(_("Victory!"),
      -- TRANSLATORS: Claus Lembeck – Victory 1
      _([[The dikes are complete at last. We have constructed a belt of solid wood several miles long all the way from the western to the eastern shore of the Kattegat, wide and sturdy enough to build houses on. I would love to see the look on Atterdag’s face when he discovers what we have done. No Danish ship will pass here anytime soon.]])
      .. paragraphdivider() ..
      -- TRANSLATORS: Claus Lembeck – Victory 1
      _([[Let us return to Helgoland now to gather our troops, and then we will reclaim our home!]]))
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
