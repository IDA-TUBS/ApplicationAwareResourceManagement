# TANOS - Timestamp-based Application-aware Network Management using Object Synchronization for Dynamic TSN 

## Requirements

The TANOS application aware network management is build with boost 1.73.0 support.

Boost library can be found here: [Boost 1.73.0](https://www.boost.org/users/history/version_1_73_0.html)

For Ubuntu some additional packages for boost are required:

```bash
sudo apt-get install zlib1g-dev libicu-dev
```

Alternative install boost with install_boost.sh in thirdparty folder

For compiling the source code cmake is mandatory

## Installation and Usage

### Install and compile
1. Clone the repository:
	```bash
	https://github.com/IDA-TUBS/ApplicationAwareResourceManagement.git
	```
	
2. Installing required software:
	```bash
	sudo apt-get install cmake
	cd thirdparty
	sudo sh install_boost.sh
	cd ..
	```
	
3. compiling sources:
	```bash
	sh build.sh
	```

### Running demo

1. modify configuration file
	```bash
	cp config/demonstrator_configuration.json ./build/bin
	```
2. executing central resource manager RM
	```bash
	cd build/bin
	./wired_rm SENSORFUSION_TEST
	```
3. executing resource manager client RM-Client
	```bash
	./wired_rm_client_endnode TEST 999
	```

## License
This project is licensed under the GNU Lesser General Public License - see the LICENSE file for details.

## Contact

Dominik Stöhrmann (stoehrmann@ida.ing.tu-bs.de) 