# Local Command-line Visualizer for Remote Machines
This command-line tool is used to visualize images and videos 
from remote machines on local machine that must have desktop system.
This tool contains two parts, server and client. 
The server is hanging on until receiving requests from the client 
which sends image or video via ZeroMQ. And the server will display it 
on local. Naturally, you should install the server part on your local machine, 
and install the client part on your remote machine.

## 1 Prerequisites and requirements
**NOTE:** The tool is only test in Ubuntu(server and client parts), 
and other linux releases should support.

The following packages should be installed in **both local machine and remote 
machine**:
- [ZeroMQ](https://github.com/zeromq/libzmq#linux): a wrapper for socket API
- [OpenCV](https://docs.opencv.org/master/d7/d9f/tutorial_linux_install.html): process, display images and videos
- [Boost](https://www.boost.org/doc/libs/1_73_0/more/getting_started/unix-variants.html): two 
components used
    - serialization: serialize and deserialize images data
    - program_options: manage command-line options

## 2 Install the server part on local machine
```bash
$ git clone https://github.com/7wdeepin/cmd_visualizer.git
$ cd ./cmd_visualizer/server
$ mkdir build && cd build
$ cmake ..
$ make 
$ sudo make install
```

## 3 Install the client part on remote machine
```bash
$ ssh remote-username@remote-hostname
$ git clone https://github.com/7wdeepin/cmd_visualizer.git
$ cd ./cmd_visualizer/client
$ mkdir build && cd build
$ cmake ..
$ make 
$ sudo make install
```

## 4 Quick start
### 4.1 Start the server
Open a terminal on local machine, and run
```bash
$ cvs
```
Or run the server in the backend
```bash
$ cvs &
```

### 4.2 Connect to the server 
Open another one terminal on local machine, and connect to the remote machine via ssh
```bash
$ ssh remote-username@remote-hostname
$ cvc /path/to/image/file
$ cvc -t image /path/to/image/file
$ cvc -t video /path/to/video/file
```
The image/video will visualize on local machine.

## 5 Modify default configurations
The configuration file **.cvsinfo** for the server part is in the path of **~/.cvsinfo** 
on local machine, and **.cvcinfo** for the client  part is in the path of **~/.cvcinfo** 
on remote machine.

And you can also pass arguments in command-line to override values of the configuration file.
Run the following command to get usage:

for server
```bash
$ cvs -h
```
for client
```bash
$ cvc -h
```