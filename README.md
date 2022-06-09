
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
# install.sh copies files to /opt/ki and puts executable in /usr/local/bin
./install.sh
```

## Install (Windows)

Install `MinGW` and make sure to install the `mingw32-gcc-bin` and `mingw32-make-bin` packages.

```bash
c:
git clone git@github.com:ki-lang/ki.git
cd ki
make
# install.bat will add c:/ki/ (install.bat directory) to your PATH (run as admin)
install.bat
```

## Http server example

```
use ki:http;

func handler(http:Request req) http:Response{

	return http:Response{
		body: "Hello from example"
	};
}

func main() i32 {

	@ s = http:Server { port: 9000, handler: handler };
	s.start() or return {
		println("Failed to start http server");
		return 1;
	};

	return 0;
}
```
