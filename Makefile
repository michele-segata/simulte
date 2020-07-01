
#
# Copyright (C) 2013-2019 Christoph Sommer <sommer@ccs-labs.org>
#
# Documentation for these modules is at http://veins.car2x.org/
#
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

.PHONY: all makefiles clean cleanall

# if out/config.py exists, we can also create command line scripts for running simulations
ADDL_TARGETS =
ifeq ($(wildcard out/config.py),)
else
    ADDL_TARGETS += run
endif

# default target
all: src/Makefile $(ADDL_TARGETS)
ifdef MODE
	@cd src && $(MAKE)
else
	@cd src && $(MAKE) MODE=release
	@cd src && $(MAKE) MODE=debug
endif

# command line scripts
run: % : src/scripts/%.in.py out/config.py
	@echo "Creating script \"./$@\""
	@sed '/# v-- contents of out\/config.py go here/r out/config.py' "$<" > "$@"
	@chmod a+x "$@"

# legacy
makefiles:
	@echo
	@echo '====================================================================='
	@echo 'Warning: make makefiles has been deprecated in favor of ./configure'
	@echo '====================================================================='
	@echo
	./configure
	@echo
	@echo '====================================================================='
	@echo 'Warning: make makefiles has been deprecated in favor of ./configure'
	@echo '====================================================================='
	@echo

clean: src/Makefile
ifdef MODE
	@cd src && $(MAKE) clean
else
	@cd src && $(MAKE) MODE=release clean
	@cd src && $(MAKE) MODE=debug clean
endif

cleanall: clean
	rm -f src/Makefile
	rm -f out/config.py
	rm -f run

src/Makefile:
	@echo
	@echo '====================================================================='
	@echo '$@ does not exist.'
	@echo 'Please run "./configure" or use the OMNeT++ IDE to generate it.'
	@echo '====================================================================='
	@echo
	@exit 1

out/config.py:
	@echo
	@echo '====================================================================='
	@echo '$@ does not exist.'
	@echo 'Please run "./configure" to generate it.'
	@echo '====================================================================='
	@echo
	@exit 1
