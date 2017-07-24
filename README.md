# cpp-tcp-client
TCP Client in C++ that connects to Python AST server and parses .json data

**Download boost Library:** http://www.boost.org/</br>


Download boost library and unzip files.
Open command prompt, change to boost_1_64_0 directory:

`$ cd D:/boost/boost_1_64_0` (in my case):

Fire bootstrap:

`$ bootstrap`

## Build boost library for x64 architecture

`$ ./b2 msvc architecture=x86 address-model=64 stage`

`$ ./b2 msvc architecture=x86 address-model=64 install`

## In Visual Studio:
Right click on project level, left click Properties:

**C/C++:**</br>
General / Additional Include Directories: `[path]\boost\boost_1_64_0`</br>
Precompile Headers / Precompile Header: Not Using Precompiled Headers

**Linker:**</br>
General / Additional Library Dependencies: `[path]\boost\boost_1_64_0\stage\lib`
