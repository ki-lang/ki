
<div align="center">
<p>
    <img width="120" src="https://raw.githubusercontent.com/ki-lang/ki/master/misc/logo-edges.png">
</p>
</div>

# Ki
ki is a programming language designed to be fast and easy to use. We want fast compile times, fast runtimes, great package management, no seg faults, easy to setup web servers and GUIs. We feel like current languages do not provide this. Either their syntax is complex (rust) or have bad package management (go, c) and most of their compile times should be alot faster (even go).

ki is focussed on making applications and servers. We are not a system programming language.

## Install (Linux / macOS)

Linux dependencies:  
Ubuntu/Debian: `sudo apt install build-essential`  
Arch: `sudo pacman -S gcc`

```bash
git clone git@github.com:ki-lang/ki.git
cd ki
make
# install.sh copies executable to /usr/bin (linux) or /usr/loca/bin (osx)
./install.sh
```

## Install (Windows)

Install `MinGW` and make sure to install the `mingw32-gcc-bin` and `mingw32-make-bin` packages.

```bash
git clone git@github.com:ki-lang/ki.git
cd ki
make
# install.bat copies executable to c:/ki/ and adds it to PATH
install.bat
```

## Http server example

```
func handler(ki:http:Request req) ki:http:Response{

	return ki:http:Response{
		body: "Hello from example"
	};
}

func main() i32 {

	@ s = ki:http:Server { port: 9000, handler: handler };
	s.start() or return 1;

	return 0;
}
```
