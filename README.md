# libwpcp

[![Build Status: Linux](https://travis-ci.org/WebProcessControl/libwpcp.svg?branch=master)](https://travis-ci.org/WebProcessControl/libwpcp)
[![Build Status: Windows](https://ci.appveyor.com/api/projects/status/1fycbouidwq72l5e/branch/master?svg=true)](https://ci.appveyor.com/project/WebProcessControl/libwpcp/branch/master)
[![Coverage Status](https://coveralls.io/repos/github/WebProcessControl/libwpcp/badge.svg?branch=master)](https://coveralls.io/github/WebProcessControl/libwpcp?branch=master)
[![MIT licensed](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

## Introduction

libwpcp is a lightweight library to allow any existing application to exchange data via the [(Reverse)WebProcessControlProtocol (RWPCP/WPCP)](http://wpcp.net). It comes with an optional implementation for [libwebsockets](https://libwebsockets.org) to minimize the coding effort.

## Building libwpcp

You need to install [CMake](https://cmake.org) and a working C compiler. Then run `cmake /path/to/source/directoy` to generate build files, which can then be compiled with `cmake --build /path/to/build/directory`.

The implementation for libwebsockets needs an installation of that library on your machine. If yo do not have one, it's possible to pass `-DWPCP_BUILD_LIBWEBSOCKETS=ON` to the first `cmake` call, which will build it for you.

For building the tests [check](https://libcheck.github.io/check/) must be installed. Alternatively you can pass `-DWPCP_BUILD_CHECK=ON` to the first `cmake` call to let it build for you.

## License

libwpcp is distributed under the OSI-approved MIT License. See [LICENSE](LICENSE) for details.
