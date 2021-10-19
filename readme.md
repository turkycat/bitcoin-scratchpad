# bitcoin-scratchpad

just a place for scribbles about bitcoin

### disclaimer

the private keys or public addresses seen within this repository should not be used by anyone who does not wish to die a very painful death.

### building

prereqs:
```
sudo apt-get install build-essentials git -y
```

(makefile coming soon)

#### gen

```
git clone http://github.com/turkycat/bitcoin-scratchpad.git
cd bitcoin-scratchpad
g++ generate-key-pair-and-address.cpp -o gen -std=c++11 $(pkg-config --cflags --libs libbitcoin-system)
```