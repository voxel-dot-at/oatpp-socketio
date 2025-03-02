# Oatpp-Socket IO Project

## Overview

The `oatpp-socketio` project is an implementation of the [Socket.IO](https://socket.io/docs/v4/socket-io-protocol/) / [Engine.IO, v4](https://socket.io/docs/v4/engine-io-protocol/) protocol in C++, specifically for [OAT++](https://oatpp.io/). 

**WIP** This project is still work in progress - engine.io has been implemented, we're working on socket.io at the moment.

## Project Structure

This uses minimal prerequisites, esp.:

- **CMake** (build project)
- **Oatpp** (used for HTTP handling)
  - **oatpp-websockets** (websockets implementation)
  - **oatpp-Swagger** (for API documentation)

## Project Setup

First, build the pre-requisites. As this has been implemented in Linux, build the oatpp libraries with the shared library option turned on.

Follow these steps to build the `toffy-oatpp` API project:

### Create a build directory and run CMake:

   ```sh
   mkdir build
   cd build
   cmake ..
   ```

### Build the project:

   ```sh
   cd ..
   make -j$(nproc) -C build install
   ```

### Run the Engine.IO server:

   ```sh
   ./build/app/engineio_server
   ```

The API will now be running and accessible. You can view the Swagger API documentation by navigating to:

   http://localhost:8000/swagger/ui#/

