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

ATTRIBUTION
===========
pakpak uses zlib to deflate/inflate files when unpacking/repacking: https://github.com/madler/zlib

LICENSE
=======
This project is licensed under the MIT License. 
