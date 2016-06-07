
#pragma once

#include "Mona/Mona.h"
#include "FlashStream.h"
#include "RTMFPConnection.h"
#include "GroupListener.h"

#define NETGROUP_MAX_PACKET_SIZE		959
#define MAX_FRAGMENT_MAP_SIZE			1024

class P2PConnection;
class NetGroup : public virtual Mona::Object {
public:
	NetGroup(const std::string& groupId, const std::string& groupTxt, const std::string& streamName, bool publisher, RTMFPConnection& conn, double updatePeriod, Mona::UInt16 windowDuration);
	virtual ~NetGroup();

	void close();

	// Add a peer to the NetGroup map
	void addPeer(const std::string& peerId, std::shared_ptr<P2PConnection> pPeer);

	// Remove a peer from the NetGroup map
	void removePeer(const std::string& peerId);

	// Set the node Id of a peer
	void updateNodeId(const std::string& id, const std::string& nodeId);

	// Send report requests (messages 0A, 22)
	void manage();

	const std::string idHex;	// Group ID in hex format
	const std::string idTxt;	// Group ID in plain text (without final zeroes)
	const std::string stream;	// Stream name
	const bool isPublisher;

private:

	void removePeer(const std::pair<std::string, std::shared_ptr<P2PConnection>>& itPeer);

	// Fragments 
	struct MediaPacket {

		MediaPacket(const Mona::PoolBuffers& poolBuffers, const Mona::UInt8* data, Mona::UInt32 size, Mona::UInt32 totalSize, Mona::UInt32 time, AMF::ContentType mediaType,
			Mona::UInt64 fragmentId, Mona::UInt8 groupMarker, Mona::UInt8 splitId);

		Mona::UInt32 payloadSize() { return pBuffer.size() - (payload - pBuffer.data()); };

		Mona::PoolBuffer	pBuffer;
		Mona::UInt32		time;
		AMF::ContentType	type;
		const Mona::UInt8*	payload; // Payload position
		Mona::UInt8			marker;
		Mona::UInt8			splittedId;
	};
	std::map<Mona::UInt64, MediaPacket>						_fragments;
	std::map<Mona::UInt32, Mona::UInt64>					_mapTime2Fragment; // Map of time to fragment (only START and DATA fragments are referenced)
	Mona::UInt64											_fragmentCounter;
	std::recursive_mutex									_fragmentMutex;

	// Erase old fragments (called before generating the fragments map)
	void	eraseOldFragments();

	// Update the fragment map
	// Return False if there is no fragments, otherwise true
	bool	updateFragmentMap();

	// Build the Group Report for the peer in parameter
	// Return false if the peer is not found
	bool	buildGroupReport(const std::string& peerId);

	// Push an arriving fragment to the peers and write it into the output file (recursive function)
	bool	pushFragment(std::map<Mona::UInt64, MediaPacket>::iterator itFragment);

	// Calculate the pull & play mode balance and send the requests if needed
	void	updatePlayMode();

	// Calculate the Best list and connect/disconnect to peers
	void	manageBestList();

	Mona::Int64												_updatePeriod; // NetStream.multicastAvailabilityUpdatePeriod equivalent in msec
	Mona::UInt16											_windowDuration; // NetStream.multicastWindowDuration equivalent in msec

	FlashEvents::OnGroupMedia::Type							onGroupMedia;
	FlashEvents::OnGroupReport::Type						onGroupReport;
	FlashEvents::OnGroupPlayPush::Type						onGroupPlayPush;
	FlashEvents::OnGroupPlayPull::Type						onGroupPlayPull;
	FlashEvents::OnFragmentsMap::Type						onFragmentsMap;
	FlashEvents::OnGroupBegin::Type							onGroupBegin;
	FlashEvents::OnFragment::Type							onFragment;
	GroupEvents::OnMedia::Type								onMedia;

	Mona::Buffer											_streamCode; // 2101 + Random key on 32 bytes to be send in the publication infos packet

	std::map<std::string,std::string>						_mapNodesId; // Map of Nodes ID to peers ID
	std::map<std::string, std::shared_ptr<P2PConnection>>	_mapPeers; // Map of peers ID to p2p connections
	GroupListener*											_pListener; // Listener of the main publication (only one by intance)
	RTMFPConnection&										_conn; // RTMFPConnection related to
	Mona::Time												_lastPlayUpdate; // last Play Pull & Push calculation
	Mona::Time												_lastBestCalculation; // last Best list calculation
	Mona::Time												_lastReport; // last Report Message time
	Mona::Time												_lastFragmentsMap; // last Fragments Map Message time
	Mona::Buffer											_reportBuffer; // Buffer for reporting messages

	bool													_firstPushMode; // True if no play push mode have been send for now
};
