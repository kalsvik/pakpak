pakpak - CLI Unpacker & Repacker for Epic Mickey PAK files.
=======================================================

HOW 2 USE
=========
Download the program from GitHub: https://github.com/kalsvik/pakpak/releases
Open a terminal program (powershell, cmd, etc).
On powershell & sh variants you can use `./<program-name>` to execute programs

UNPAK
-----
The command to unpack content from a pak file is `./<pakpak-name> unpak <pakfile.pak>`.
pakpak will then create a new folder in the working directory called '<pakfile.pak>-unpak', where you can browse its contents.

REPAK
-----
The command to repack content to a pak file is `./<pakpak-name> pak <pakfile.pak>-unpak` 
pakpak will then create a new file in the working directory called 'out.pak', from there you can rename it to the pak you're replacing and chuck it into the game.

ADDING FILES TO PAK
-------------------
To add a file to a pak, you'll need to first have a pakpak-unpak directory with a '.pakpak-order' file at the root. This file dictates what files/folders go in the pak and in what order.
The basic syntax works like this:

######################################## -> !**All these folders & files must exist in the actual unpak. & you must not include spaces anywhere in the names.**!
# D my-folder		               # -> Folders are defined by the D prefix. Every file definition under this line can be found in this folder.
# F my-file-in-folder.nif NIF          # -> Files are defined by the F prefix and have their extension printed after the path name. This is required.
# D my-other-folder	               # 
# F my-file-in-other-folder1.txt TXT   # -> You can add more than one file line per folder definition.
# FC my-file-in-other-folder2.bin BIN  # -> Compressed files are defined by the FC prefix.
########################################

This same structure mimics the one found in the actual unpak.
To add new files, its generally good practice to make your own folder and add it to the bottom of the '.pakpak-order' file unless it HAS to be somewhere else. 

########################################
# D my-new-folder		       # -> Remember to always include extensions!
# F my-new.nif NIF	               # 	
# FC my-new.bin BIN		       #
# F my-new.txt TXT		       #
# F my-new.hkx_wii HKX		       #
########################################

You can then refer to the 'REPAK' section to repack the contents with your new stuff.

ATTRIBUTION
===========
pakpak uses zlib to deflate/inflate files when unpacking/repacking: https://github.com/madler/zlib

LICENSE
=======
This project is licensed under the MIT License. 
