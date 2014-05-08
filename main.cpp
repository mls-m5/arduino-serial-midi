#include "alsasequencer.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <termio.h>


//From: https://github.com/todbot/arduino-serial
// takes the string name of the serial port (e.g. "/dev/tty.usbserial","COM1")
// and a baud rate (bps) and connects to that port at that speed and 8N1.
// opens the port in fully raw mode so you can send binary data.
// returns valid fd, or -1 on error
int serialport_init(const char* serialport, int baud)
{
    struct termios toptions;
    int fd;

    //fd = open(serialport, O_RDWR | O_NOCTTY | O_NDELAY);
    fd = open(serialport, O_RDWR | O_NONBLOCK );

    if (fd == -1)  {
        perror("serialport_init: Unable to open port ");
        return -1;
    }

    //int iflags = TIOCM_DTR;
    //ioctl(fd, TIOCMBIS, &iflags);     // turn on DTR
    //ioctl(fd, TIOCMBIC, &iflags);    // turn off DTR

    if (tcgetattr(fd, &toptions) < 0) {
        perror("serialport_init: Couldn't get term attributes");
        return -1;
    }
    speed_t brate = baud; // let you override switch below if needed
    switch(baud) {
    case 4800:   brate=B4800;   break;
    case 9600:   brate=B9600;   break;
#ifdef B14400
    case 14400:  brate=B14400;  break;
#endif
    case 19200:  brate=B19200;  break;
#ifdef B28800
    case 28800:  brate=B28800;  break;
#endif
    case 38400:  brate=B38400;  break;
    case 57600:  brate=B57600;  break;
    case 115200: brate=B115200; break;
    }
    cfsetispeed(&toptions, brate);
    cfsetospeed(&toptions, brate);

    // 8N1
    toptions.c_cflag &= ~PARENB;
    toptions.c_cflag &= ~CSTOPB;
    toptions.c_cflag &= ~CSIZE;
    toptions.c_cflag |= CS8;
    // no flow control
    toptions.c_cflag &= ~CRTSCTS;

    //toptions.c_cflag &= ~HUPCL; // disable hang-up-on-close to avoid reset

    toptions.c_cflag |= CREAD | CLOCAL;  // turn on READ & ignore ctrl lines
    toptions.c_iflag &= ~(IXON | IXOFF | IXANY); // turn off s/w flow ctrl

    toptions.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // make raw
    toptions.c_oflag &= ~OPOST; // make raw

    // see: http://unixwiz.net/techtips/termios-vmin-vtime.html
    toptions.c_cc[VMIN]  = 0;
    toptions.c_cc[VTIME] = 0;
    //toptions.c_cc[VTIME] = 20;

    tcsetattr(fd, TCSANOW, &toptions);
    if( tcsetattr(fd, TCSAFLUSH, &toptions) < 0) {
        perror("init_serialport: Couldn't set term attributes");
        return -1;
    }

    return fd;
}

int serialport_flush(int fd)
{
    sleep(2); //required to make flush work, for some reason
    return tcflush(fd, TCIOFLUSH);
}

void printHelp(){
	printf("Usage: serialmidi [options]\n\n"
			"serialmidi - a program to send arduino serial data as midi messages by Mattias Lasersköld\n\n"
			"Options:\n\n"
			"\t-o\n"
			"\t\tPrint to stdout only\n\n"
			"\t-h\n"
			"\t\tPrint this help\n\n"
			"\t-b\n"
			"\t\tSet baudrate\n\n"
			"\t-c\n"
			"\t\tSet control number\n\n"
			"\t-p\n"
			"\t\tset port name\n\n");
}

using std::cout; using std::endl;
using std::stringstream; using std::string;

int main(int argc, char **argv) {
	AlsaSequencer::StartSequencer("serialmidi");
	int baudRate = 9600;
	string portName = "/dev/ttyACM0";
	bool printToStdoutOnly = false;
	int midiControlNumber = 2;

	for (int i = 0; i < argc; ++i){
		string arg = argv[i];
		if (arg.compare("-b") == 0){
			stringstream ss;
			++i;
			ss << argv[i];
			ss >> baudRate;
			if (baudRate <= 0){
				cout << "wrong baudrate: " << baudRate << endl;
				return 0;
			}
		}
		if (arg.compare("-p") == 0){
			++i;
			portName = argv[i];
		}
		if (arg.compare("-o") == 0){
			printToStdoutOnly = true;
		}
		if (arg.compare("-h") == 0){
			printHelp();
			return 0;
		}
		if (arg.compare("-c") == 0){
			stringstream ss;
			++i;
			ss << argv[i];
			ss >> midiControlNumber;
		}
	}

	int file = serialport_init(portName.c_str(), baudRate);
	if (file == -1){
		std::cerr << "could not open " << portName << " exiting..." << endl;
		return 1;
	}
	if (!printToStdoutOnly){
		cout << "started... baudrate: " << baudRate << " port " << endl;
	}

	int value = 0, oldvalue = 0;
	char ch;
	char buf[100];
	int writePos = 0;
	int timeout = 1000000;
	while (timeout > 0){
		value = -1;

		int n = read(file, &ch, 1);  // read a char at a time
		if( n==-1) return -1;    // couldn't read
		if( n==0 ) {
			usleep( 1 * 1000 );  // wait 1 msec try again
			timeout--;
			continue;
		}
		else {
			timeout = 1000000;
		}

		if (ch >= '0' && ch <= '9'){
			buf[writePos] = ch;
			++writePos;
		}
		else if(writePos > 0){
			stringstream ss;
			buf[writePos] = 0; //End of string

			ss << buf;
			ss >> value;

			writePos = 0;

			if (printToStdoutOnly){
				cout << value;
			}
			else if (value != oldvalue){
				if (value < 128){
					AlsaSequencer::sendEvent(10, midiControlNumber, value, false);
				}
				oldvalue = value;
			}
		}
	}


	cout << "stopped..." << endl;
	return 0;
}
