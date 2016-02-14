# Simplest possible makefile, added for github source release.
# I wrote this on Windows, and in fact it didn't even compile on *nix
# without a lot of tweaking due to relying on nonstandard MSVC extensions.
all:
	g++ *.cpp modules/*.cpp utils/*.cpp -o heerip
