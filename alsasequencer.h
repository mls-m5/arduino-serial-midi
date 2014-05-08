/*
 * alsasequencer.h
 *
 *  Created on: 19 aug 2013
 *      Author: mattias
 */

#ifndef ALSASEQUENCER_H_
#define ALSASEQUENCER_H_
#include <alsa/asoundlib.h>
#include <string>
#include <list>

class AlsaSequencer {
public:
	virtual ~AlsaSequencer();
	static void StartSequencer(std::string portname);

	static void sendEvent(int type, int note, int value, bool internal = false);
	static AlsaSequencer *GetSequencer();

protected:
	AlsaSequencer(std::string portname);
	static void* listenThread(void *);
	void StartThread();

private:
	int openSequencer(snd_seq_t **seqHandle, std::string portname, int inPorts[], int outPorts[],
			int numIn, int numOut);
	void handleEvent(snd_seq_event_t* ev);

	snd_seq_t *seqHandle;
};

#endif /* ALSASEQUENCER_H_ */
