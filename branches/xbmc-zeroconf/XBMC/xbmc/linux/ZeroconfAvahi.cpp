#if (defined(_LINUX) || ! defined(__APPLE__))

#import "ZeroconfAvahi.h"
#include <string>
#include <iostream>
#include <sstream>
#include <avahi-client/client.h>
#include <avahi-common/thread-watch.h>
#include <avahi-common/error.h>
#include <unistd.h> //gethostname

//TODO: fix log output to xbmc methods
//TODO: throw exception in constructor?

///helper struct to block event loop for modifications
struct ScopedEventLoopBlock{
	ScopedEventLoopBlock(AvahiThreadedPoll* fp_poll):mp_poll(fp_poll){
		avahi_threaded_poll_lock(mp_poll);
	}
	~ScopedEventLoopBlock(){
		avahi_threaded_poll_unlock(mp_poll);
	}
	private:
		AvahiThreadedPoll* mp_poll;
};



CZeroconfAvahi::CZeroconfAvahi(): mp_client(0), mp_poll (0), mp_webserver_group(0){
	if(! (mp_poll = avahi_threaded_poll_new())){
		std::cerr << "Ouch. Could not create threaded poll object" << std::endl;
		return;
	}
	mp_client = avahi_client_new(avahi_threaded_poll_get(mp_poll), 
				AVAHI_CLIENT_NO_FAIL, &clientCallback,0,0);
	if(!mp_client)
	{
		std::cerr << "Ouch. Failed to create avahi client" << std::endl;
		mp_client = 0;
		return;
	}
	//start event loop thread
	if(avahi_threaded_poll_start(mp_poll) < 0){
		std::cerr << "Ouch. Failed to start avahi client thread" << std::endl;
	}
}

CZeroconfAvahi::~CZeroconfAvahi(){
		avahi_threaded_poll_stop(mp_poll);
		if(mp_webserver_group) avahi_entry_group_free(mp_webserver_group);
		avahi_client_free(mp_client);
		avahi_threaded_poll_free(mp_poll);
}

void CZeroconfAvahi::doPublishWebserver(int f_port){
	ScopedEventLoopBlock l_block(mp_poll);
	
	if(mp_webserver_group)
		avahi_entry_group_reset(mp_webserver_group);
	if(
	if (!(mp_webserver_group = avahi_entry_group_new(mp_client, &CZeroconfAvahi::groupCallback, NULL))){
		std::cerr << "avahi_entry_group_new() failed:" << avahi_strerror(avahi_client_errno(mp_client)) << std::endl;
		return;
	}
		
	if (avahi_entry_group_is_empty(mp_webserver_group)) {
		int ret;
		if ((ret = avahi_entry_group_add_service(mp_webserver_group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, AvahiPublishFlags(0), assembleWebserverServiceName().c_str(), 
			 "_http._tcp", NULL, NULL, f_port, NULL) < 0))
		{

			if (ret == AVAHI_ERR_COLLISION){
				std::cerr << "Ouch name collision :/ FIXME!!" << std::endl; //TODO
				return;
			}

			std::cerr << "Failed to add _http._tcp service: "<< avahi_strerror(ret) << std::endl;
			return;
		}
		/* Tell the server to register the service */
		if ((ret = avahi_entry_group_commit(mp_webserver_group)) < 0) {
			std::cerr << "Failed to commit entry group: " << avahi_strerror(ret) << std::endl;
		}
	}
}

void  CZeroconfAvahi::doRemoveWebserver(){
	if(!mp_webserver_group){
		std::cerr << "Webserver not published. Nothing todo" << std::endl;
		return;
	} else {
		if(mp_webserver_group)
			avahi_entry_group_reset(mp_webserver_group);
		mp_webserver_group = 0;
	}
}

void CZeroconfAvahi::doStop(){
	doRemoveWebserver();
}

void CZeroconfAvahi::clientCallback(AvahiClient* fp_client, AvahiClientState f_state, void*){
	
	switch(f_state){
		case AVAHI_CLIENT_S_RUNNING:
			std::cout << "Client's up and running!" << std::endl;
			//NOW DO REGISTER
			break;
		case AVAHI_CLIENT_FAILURE:
			std::cerr << "Avahi client failure: " << avahi_strerror(avahi_client_errno(fp_client)) << std::endl;
			//TODO
			//We were forced to disconnect from server. now recreate the client object
			break;
		case AVAHI_CLIENT_S_COLLISION:
		case AVAHI_CLIENT_S_REGISTERING:
			//HERE WE SHOULD REMOVE ALL OF OUR SERVICES AND "RESCHEDULE" them for later addition
			break;
		case AVAHI_CLIENT_CONNECTING:
			//SERVER is not available, but may become later
			break;
	}
}

void CZeroconfAvahi::groupCallback(AvahiEntryGroup *fp_group, AvahiEntryGroupState f_state, void *){
	switch (f_state) {
		case AVAHI_ENTRY_GROUP_ESTABLISHED :
			/* The entry group has been established successfully */
			std::cerr << "Service successfully established." << std::endl;
			break;

		case AVAHI_ENTRY_GROUP_COLLISION :
			std::cerr << "Service name collision... FIXME!." << std::endl;
			//TODO handle collision rename, etc and directly readd them; don't free, but just change name and recommit
			break;

		case AVAHI_ENTRY_GROUP_FAILURE :
			std::cerr << "Entry group failure: "<< avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(fp_group))) << " FIXME!!" << std::endl;		
			//Some kind of failure happened while we were registering our services <--- and that means exactly what!?!
			break;

		case AVAHI_ENTRY_GROUP_UNCOMMITED:
		case AVAHI_ENTRY_GROUP_REGISTERING:
		;
	}
}

std::string CZeroconfAvahi::assembleWebserverServiceName(){
	std::stringstream ss;
	ss << GetWebserverPublishPrefix() << '@';
	
	//get our hostname
	char lp_hostname[256];
	if(gethostname(lp_hostname, sizeof(lp_hostname))){
		//TODO
		std::cerr << "could not get hostname.. hm... waaaah! PANIC! " << std::endl;
		ss << "DummyThatCantResolveItsName";
	} else {
		ss << lp_hostname;
	}
	return ss.str();

}


#endif