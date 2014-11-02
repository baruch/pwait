#    Copyright (C) 2014 by David Zaslavsky <diazona@ellipsix.net>

#   This program is free software; you can redistribute it and/or modify
#   it under the terms of version 2 of the GNU General Public License as
#   published by the Free Software Foundation.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the
#   Free Software Foundation, Inc.,
#   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .

project(pwait)

cmake_minimum_required(VERSION 2.8)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(CheckSymbolExists)
check_symbol_exists(__GLIBC__ features.h _GNU_SOURCE)
configure_file(config.h.in config.h)

add_executable(pwait pwait.c ptrace.c netlink.c poll.c capabilities.c)
target_link_libraries(pwait cap)

install(TARGETS pwait RUNTIME DESTINATION bin)
install(CODE "execute_process(COMMAND setcap cap_sys_ptrace+p cap_net_admin+p pwait WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin)")
