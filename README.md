## About
Nextcloud tor client.

## Compile
**Debian 12**
```
sudo apt-get install libcurl4-openssl-dev
sudo apt install tor
sudo service tor restart
```

**Arch 2023**
```
sudo pacman -S curl tor
sudo systemctl restart tor
```

**And**
```
Edit: /etc/tor/torrc
SocksPort 9050 # Proxy port
SocksListenAddress 127.0.0.1 # IP for listen (local)

git clone https://github.com/oldteamhost/Bridge.git
cd bridge
make
```
