# Call from gun shoot function — writes trigger for RecoilExpand native mod
# Example: after shoot logic in akmquantity.mcfunction add:
#   function recoilexpand/fire_akm
# Or inline the /system write if available.

# Bedrock cannot write arbitrary sdcard from mcfunction easily.
# Prefer Script API bridge below.
