#	$NetBSD: std.arc,v 1.9 2000/01/25 16:07:13 enami Exp $
# standard arc info

machine arc mips

prefix ../gnu/sys
cinclude "conf/files.softdep"
prefix

mainbus0 at root
cpu* at mainbus0

# set CPU architecture level for kernel target
#options 	MIPS1			# R2000/R3000 support
options 	MIPS3			# R4000/R4400 support

# Standard (non-optional) system "options"

# Standard exec-package options
options 	EXEC_ELF32		# native exec format
options 	EXEC_SCRIPT		# may be unsafe

options 	MIPS3_L2CACHE_PRESENT	# may have L2 cache
options 	MIPS3_L2CACHE_ABSENT	# may not have L2 cache

options 	__NO_SOFT_SERIAL_INTERRUPT	# for "com" driver
