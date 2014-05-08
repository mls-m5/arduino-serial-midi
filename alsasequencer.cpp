/*
 * alsasequencer.cpp
 *
 *  Created on: 19 aug 2013
 *      Author: mattias
 */

#include "alsasequencer.h"

#include <iostream>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>

using std::cout; using std::endl;
using std::string;

static AlsaSequencer * globalSequencer = 0;

int AlsaSequencer::openSequencer(snd_seq_t **seqHandle, std::string portname, int inPorts[], int outPorts[],
		int numIn, int numOut){

	int l1;
	string tportname;

	if (snd_seq_open(seqHandle, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0){

		cout << "Kunde inte öppna alsa-sequencern" << endl;
		return -1;
	}
	snd_seq_set_client_name(*seqHandle, portname.c_str());
	for (l1 = 0; l1 < numIn; ++l1){
		tportname = string("in ") + (char)('a' + l1);
		if ((inPorts[l1] = snd_seq_create_simple_port(*seqHandle, tportname.c_str(),
				SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
				SND_SEQ_PORT_TYPE_APPLICATION)) < 0){
			std::cerr << "Kunde inte öppna sequencerport" << endl;
			return -1;
		}
	}

	for (l1 = 0; l1 < numOut; ++l1){
		tportname = string("out ") + (char)('a' + l1);
		if ((outPorts[l1] = snd_seq_create_simple_port(*seqHandle,
				tportname.c_str(), SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
				SND_SEQ_PORT_TYPE_APPLICATION)) < 0){
			std::cerr << "Kunde inte öppna sequencer-port" << endl;
			return -1;
		}
	}
	return 0;
}


void* AlsaSequencer::listenThread (void * arg)
{
	auto seq = (AlsaSequencer*) arg;
	seq->StartThread();
	return 0;
}

void AlsaSequencer::StartSequencer(string portname) {
	pthread_t t;
	if (globalSequencer){
		delete globalSequencer;
	}

	globalSequencer = new AlsaSequencer(portname);
	void * pointer = (void * ) globalSequencer;
	pthread_create(&t ,
			NULL,
			listenThread,
			pointer);
}

void AlsaSequencer::StartThread(){
	int npfd;
	struct pollfd *pfd;
	npfd = snd_seq_poll_descriptors_count(seqHandle, POLLIN);
	pfd = (struct pollfd *) alloca (npfd * sizeof(struct pollfd));
	snd_seq_poll_descriptors(seqHandle, pfd, npfd, POLLIN);
	while (1){
		if (poll(pfd, npfd, 100000) > 0){
			snd_seq_event_t *ev;
			snd_seq_event_input(seqHandle, &ev);

//			timeval tv;
//			gettimeofday(&tv, 0);
//			ev->time.time.tv_sec = tv.tv_sec;
//			ev->time.time.tv_nsec = tv.tv_usec * 1000L;

			handleEvent(ev);

			pthread_yield();
		}
	}
}

void AlsaSequencer::handleEvent(snd_seq_event_t* ev) {
	if (ev->type == SND_SEQ_EVENT_NOTEON){
		if(ev->data.note.velocity == 0){
			ev->type = SND_SEQ_EVENT_NOTEOFF;
		}
	}

	//Unhandled in this implementation
//	for (auto listener: listeners){
//		listener->pushEvent(ev);
//	}
}

void AlsaSequencer::sendEvent(int type, int note, int value, bool internal) {
	snd_seq_event_t ev;
	snd_seq_ev_clear(&ev);
	snd_seq_ev_set_source(&ev, 1);
	snd_seq_ev_set_subs(&ev);
	snd_seq_ev_set_direct(&ev);
	if (type == SND_SEQ_EVENT_CONTROLLER){
		snd_seq_ev_set_controller((&ev), 0, note, value);
	}
	else if (type == 144){
		snd_seq_ev_set_noteon((&ev), 0, note, value);
	}
	else if (type == 128){
		snd_seq_ev_set_noteoff((&ev), 0, note, value);
	}

	if (internal){
		globalSequencer->handleEvent(&ev);
	}
	else
	{
		snd_seq_event_output(globalSequencer->seqHandle, &ev);
		snd_seq_drain_output(globalSequencer->seqHandle);
	}
}

AlsaSequencer::AlsaSequencer(std::string portname) {
	const int midiPorts = 1;
	int inPorts[midiPorts], outPorts[midiPorts];

	if (openSequencer(&seqHandle, portname, inPorts, outPorts, midiPorts, midiPorts) < 0){
		std::cerr << "Alsa-midi-fel" << endl;
		return ;
	}

	globalSequencer = this;
}

AlsaSequencer::~AlsaSequencer() {
	snd_seq_close(seqHandle);
}

AlsaSequencer* AlsaSequencer::GetSequencer() {
	return globalSequencer;
}
