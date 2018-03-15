#include "MessageIdentifiers.h"
#include "RakPeerInterface.h"
#include "BitStream.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <map>
#include <mutex>

static unsigned int SERVER_PORT = 65000;
static unsigned int MAX_CONNECTIONS = 2;
unsigned int playerNumber;

enum NetworkState
{
	NS_Init = 0,
	NS_PendingStart,
	NS_Started,
	NS_Lobby,
	NS_Select,
	NS_Player_One,
	NS_Player_Two,
	NS_Player_Three,
	NS_Pending,
};

bool isRunning = true;

RakNet::RakPeerInterface *g_rakPeerInterface = nullptr;
RakNet::SystemAddress g_serverAddress;

std::mutex g_networkState_mutex;
NetworkState g_networkState = NS_Init;

enum {
	ID_THEGAME_LOBBY_READY = ID_USER_PACKET_ENUM,
	ID_PLAYER_READY,
	ID_THEGAME_START,
	ID_PLAYER_ONE,
	ID_PLAYER_TWO,
	ID_PLAYER_THREE,
	ID_NETWORK_STATUS_CHANGE,
};

enum EPlayerClass
{
	None = 0,
	Mage,
	Rogue,
	Warrior,
};

struct SPlayer
{
	std::string m_name;
	unsigned int m_health;
	unsigned int m_damage;
	unsigned int m_defense;
	EPlayerClass m_class = None;
	bool m_ready;
	bool is_turn;
	bool is_defending;

	//function to send a packet with name/health/class etc
	void SendName(RakNet::SystemAddress systemAddress, bool isBroadcast)
	{
		RakNet::BitStream writeBs;
		writeBs.Write((RakNet::MessageID)ID_PLAYER_READY);
		RakNet::RakString name(m_name.c_str());
		writeBs.Write(name);

		//returns 0 when something is wrong
		assert(g_rakPeerInterface->Send(&writeBs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, systemAddress, isBroadcast));
	}
};

std::map<unsigned long, SPlayer> m_players;
std::map<unsigned long, RakNet::SystemAddress> m_addresses;

SPlayer& GetPlayer(RakNet::RakNetGUID raknetId)
{
	unsigned long guid = RakNet::RakNetGUID::ToUint32(raknetId);
	std::map<unsigned long, SPlayer>::iterator it = m_players.find(guid);
	assert(it != m_players.end());
	return it->second;
}

RakNet::SystemAddress GetAddress(unsigned long id)
{
	std::map<unsigned long, RakNet::SystemAddress>::iterator it = m_addresses.find(id);
	assert(it != m_addresses.end());
	return it->second;
}

void OnLostConnection(RakNet::Packet* packet)
{
	SPlayer& lostPlayer = GetPlayer(packet->guid);
	lostPlayer.SendName(RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
	unsigned long keyVal = RakNet::RakNetGUID::ToUint32(packet->guid);
	m_players.erase(keyVal);
}

//server
void OnIncomingConnection(RakNet::Packet* packet)
{
	//must be server in order to recieve connection
	m_players.insert(std::make_pair(RakNet::RakNetGUID::ToUint32(packet->guid), SPlayer()));
	m_addresses.insert(std::make_pair(RakNet::RakNetGUID::ToUint32(packet->guid), packet->systemAddress));
	std::cout << "Total Players: " << m_players.size() << std::endl;
}

void OnLobbyReady(RakNet::Packet* packet)
{
	playerNumber++;
	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID messageId;
	bs.Read(messageId);
	RakNet::RakString userName;
	bs.Read(userName);

	unsigned long guid = RakNet::RakNetGUID::ToUint32(packet->guid);
	SPlayer& player = GetPlayer(packet->guid);
	player.m_name = userName;
	player.m_ready = true;
	std::cout << "Player " << playerNumber << " aka " << player.m_name.c_str() << " IS READY!!!!!" << std::endl;

	//notify all other connected players that this plyer has joined the game
	int counter = 0;
	for (std::map<unsigned long, SPlayer>::iterator it = m_players.begin(); it != m_players.end(); ++it)
	{

		SPlayer& player = it->second;
		if (player.m_ready)
		{
			counter++;

			if (counter == MAX_CONNECTIONS)
			{
				//std::cout << "Sending change network packet" << std::endl;
				RakNet::BitStream bs;
				bs.Write((RakNet::MessageID)ID_NETWORK_STATUS_CHANGE);
				NetworkState gState(NS_Select);
				bs.Write(gState);

				//returns 0 when something is wrong
				assert(g_rakPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true));
			}
		}
		//skip over the player who just joined
		if (guid == it->first)
		{
			continue;
		}
		player.SendName(packet->systemAddress, false);
	}
	player.SendName(packet->systemAddress, true);
}

void SetPlayerStats(RakNet::Packet * packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID messageId;
	bs.Read(messageId);
	RakNet::RakString cSelect;
	bs.Read(cSelect);

	unsigned long guid = RakNet::RakNetGUID::ToUint32(packet->guid);
	SPlayer& player = GetPlayer(packet->guid);

	std::string uClass;
	if (cSelect == "r")
	{
		player.m_class = Rogue;
		player.m_health = 40;
		player.m_damage = 10;
		player.m_defense = 10;
		uClass = "Rogue";
	}
	if (cSelect == "w")
	{
		player.m_class = Warrior;
		player.m_health = 50;
		player.m_damage = 8;
		player.m_defense = 12;
		uClass = "Warrior";
	}
	if (cSelect == "m")
	{
		player.m_class = Mage;
		player.m_health = 30;
		player.m_damage = 12;
		player.m_defense = 8;
		uClass = "Mage";
	}

	std::cout << player.m_name.c_str() << " has selected the " << uClass.c_str() << std::endl;

	RakNet::BitStream ws;
	ws.Write((RakNet::MessageID)ID_THEGAME_START);
	RakNet::RakString pClass(uClass.c_str());
	ws.Write(pClass);
	RakNet::RakString pName(player.m_name.c_str());
	ws.Write(pName);

	//returns 0 when something is wrong
	assert(g_rakPeerInterface->Send(&ws, HIGH_PRIORITY, RELIABLE_ORDERED, 0, RakNet::UNASSIGNED_SYSTEM_ADDRESS, true));

	int counter = 0;
	for (std::map<unsigned long, SPlayer>::iterator it = m_players.begin(); it != m_players.end(); ++it)
	{
		SPlayer& player = it->second;
		if (player.m_class == None)
		{
			counter++;
		}
	}

	std::cout << "Counter: " << counter << std::endl;

	if (counter == 0)
	{

		std::cout << "Somehowin here? " << counter << std::endl;
		RakNet::BitStream bs;
		bs.Write((RakNet::MessageID)ID_NETWORK_STATUS_CHANGE);
		NetworkState gState(NS_Player_One);
		bs.Write(gState);

		//returns 0 when something is wrong
		std::map<unsigned long, SPlayer>::iterator it = m_players.begin();
		assert(g_rakPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, GetAddress(it->first) , true));

	}
}

void ResolvePlayerOne(RakNet::Packet* packet)
{

	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID messageId;
	bs.Read(messageId);
	RakNet::RakString aSelect;
	bs.Read(aSelect);

	unsigned long guid = RakNet::RakNetGUID::ToUint32(packet->guid);
	SPlayer& player = GetPlayer(packet->guid);

	std::string uClass;
	if (aSelect == "1")
	{

	}
	if (aSelect == "2")
	{

	}
	if (aSelect == "3")
	{
		player.m_health += player.m_defense / 2;
		player.is_defending = true;
	}
	if (aSelect == "4")
	{

	}
	if (aSelect == "5")
	{

	}
}

unsigned char GetPacketIdentifier(RakNet::Packet *packet)
{
	if (packet == nullptr)
		return 255;

	if ((unsigned char)packet->data[0] == ID_TIMESTAMP)
	{
		RakAssert(packet->length > sizeof(RakNet::MessageID) + sizeof(RakNet::Time));
		return (unsigned char)packet->data[sizeof(RakNet::MessageID) + sizeof(RakNet::Time)];
	}
	else
		return (unsigned char)packet->data[0];
}

bool HandleLowLevelPackets(RakNet::Packet* packet)
{
	bool isHandled = true;
	// We got a packet, get the identifier with our handy function
	unsigned char packetIdentifier = GetPacketIdentifier(packet);

	// Check if this is a network message packet
	switch (packetIdentifier)
	{
	case ID_DISCONNECTION_NOTIFICATION:
		// Connection lost normally
		printf("ID_DISCONNECTION_NOTIFICATION\n");
		break;
	case ID_ALREADY_CONNECTED:
		// Connection lost normally
		printf("ID_ALREADY_CONNECTED with guid %" PRINTF_64_BIT_MODIFIER "u\n", packet->guid);
		break;
	case ID_INCOMPATIBLE_PROTOCOL_VERSION:
		printf("ID_INCOMPATIBLE_PROTOCOL_VERSION\n");
		break;
	case ID_REMOTE_DISCONNECTION_NOTIFICATION: // Server telling the clients of another client disconnecting gracefully.  You can manually broadcast this in a peer to peer enviroment if you want.
		printf("ID_REMOTE_DISCONNECTION_NOTIFICATION\n");
		break;
	case ID_REMOTE_CONNECTION_LOST: // Server telling the clients of another client disconnecting forcefully.  You can manually broadcast this in a peer to peer enviroment if you want.
		printf("ID_REMOTE_CONNECTION_LOST\n");
		break;
	case ID_NEW_INCOMING_CONNECTION:
		//client connecting to server
		OnIncomingConnection(packet);
		printf("ID_NEW_INCOMING_CONNECTION\n");
		break;
	case ID_REMOTE_NEW_INCOMING_CONNECTION: // Server telling the clients of another client connecting.  You can manually broadcast this in a peer to peer enviroment if you want.
		OnIncomingConnection(packet);
		printf("ID_REMOTE_NEW_INCOMING_CONNECTION\n");
		break;
	case ID_CONNECTION_BANNED: // Banned from this server
		printf("We are banned from this server.\n");
		break;
	case ID_CONNECTION_ATTEMPT_FAILED:
		printf("Connection attempt failed\n");
		break;
	case ID_NO_FREE_INCOMING_CONNECTIONS:
		// Sorry, the server is full.  I don't do anything here but
		// A real app should tell the user
		printf("ID_NO_FREE_INCOMING_CONNECTIONS\n");
		break;

	case ID_INVALID_PASSWORD:
		printf("ID_INVALID_PASSWORD\n");
		break;

	case ID_CONNECTION_LOST:
		// Couldn't deliver a reliable packet - i.e. the other system was abnormally
		// terminated
		printf("ID_CONNECTION_LOST\n");
		OnLostConnection(packet);
		break;

	
	case ID_CONNECTED_PING:
	case ID_UNCONNECTED_PING:
		printf("Ping from %s\n", packet->systemAddress.ToString(true));
		break;
	default:
		isHandled = false;
		break;
	}
	return isHandled;
}

void PacketHandler()
{
	while (isRunning)
	{
		for (RakNet::Packet* packet = g_rakPeerInterface->Receive(); packet != nullptr; g_rakPeerInterface->DeallocatePacket(packet), packet = g_rakPeerInterface->Receive())
		{
			if (!HandleLowLevelPackets(packet))
			{
				//our game specific packets
				unsigned char packetIdentifier = GetPacketIdentifier(packet);
				switch (packetIdentifier)
				{
				case ID_THEGAME_LOBBY_READY:
					OnLobbyReady(packet);
					break;
				case ID_THEGAME_START:
					SetPlayerStats(packet);
					break;
				case ID_PLAYER_ONE:
					ResolvePlayerOne(packet);
					break;
				default:
					break;
				}
			}
		}

		std::this_thread::sleep_for(std::chrono::microseconds(1));
	}
}

int main()
{
	g_rakPeerInterface = RakNet::RakPeerInterface::GetInstance();
	std::thread packetHandler(PacketHandler);
	g_networkState = NS_PendingStart;
	while (isRunning)
	{
		if (g_networkState == NS_PendingStart)
		{
			RakNet::SocketDescriptor socketDescriptors[1];
			socketDescriptors[0].port = SERVER_PORT;
			socketDescriptors[0].socketFamily = AF_INET; // Test out IPV4

			bool isSuccess = g_rakPeerInterface->Startup(MAX_CONNECTIONS, socketDescriptors, 1) == RakNet::RAKNET_STARTED;
			assert(isSuccess);
			//ensures we are server
			g_rakPeerInterface->SetMaximumIncomingConnections(MAX_CONNECTIONS);
			std::cout << "server started" << std::endl;
			g_networkState_mutex.lock();
			g_networkState = NS_Started;
			g_networkState_mutex.unlock();
		}
	}

	packetHandler.join();
	return 0;
}