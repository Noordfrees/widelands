function ware_helptext(tribe)
   local helptext = {
      -- TRANSLATORS: Helptext for a ware: cannonball
      default = pgettext("ware", "Cannon balls are used by ships in naval warfare to destroy one another."),
   }
   local result = ""
   if tribe then
      result = helptext[tribe]
   else
      result = helptext["default"]
   end
   if (result == nil) then result = "" end
   return result
end
