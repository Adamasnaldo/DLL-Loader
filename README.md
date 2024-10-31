# DLL-Loader
Simple program written in C++ to start a program and inject DLLs, or inject them into already running processes.

# Table of Contents
1. [Solution Structure](#solution-structure)
2. [Get Started](#get-started)
3. [Configuration](#configuration)
4. [Building From Source](#building-from-source)
5. [VCPKG Libraries](#vcpkg-libraries)

<a id="solution-structure"></a>
## 📁 Solution Structure
- [`DLL Loader`](DLL%20Loader):

  This is the main project, with the actual injector.

- [`DummyEXE`](DummyEXE):

  This is a dummy executable file. All it does is print some messages for a few seconds. It can be used to test the [DLL Loader](DLL Loader) output file, to verify if it's injecting correctly.

- [`DummyLL`](DummyLL):

  This is a dummy DLL file. It simply prints out a message to both the console and a file. It can be used to test the [DLL Loader](DLL Loader) injection.

<a id="get-started"></a>
## 🛠 Get Started
1. Download the latest [release](https://github.com/Adamasnaldo/DLL-Loader/releases/latest)
2. Copy the `config_default.toml` into a new config file. I recommend naming it `config_<name>.toml`, where `<name>` is the name of the target program, but you can name it whatever you want.
3. Edit the newly created config file with your own values.

<a id="configuration"></a>
## ⚙ Configuration
The configuration file is a [toml](https://toml.io/) file and contains all the necessary information for the program to run.
This repository contains a default config file (you can find it [here](config_default.toml)) that has all the accepted config values, and their explanation.

<a id="building-from-source"></a>
## 🏗️ Building From Source

### Requirements
- Visual Studio 2022 (might work on previous versions)

### Steps
1. Open the [solution file](DLL Stuff.sln) in Visual Studio.
2. Build the `DLL Loader` project.
3. The output exe will be in the default Visual Studio out folder (usually `x64\<Debug|Release>\DLL Loader.exe`)

<a id="vcpkg-libraries"></a>
## 📦 VCPKG Libraries
This project uses the following packages from [vcpkg](https://vcpkg.io/):
1. [spdlog](https://github.com/gabime/spdlog)
2. [tomlplusplus](https://marzer.github.io/tomlplusplus/)
