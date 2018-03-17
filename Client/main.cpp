#include "MessageIdentifiers.h"
#include "RakPeerInterface.h"
#include "BitStream.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <map>
#include <mutex>

static unsigned int SERVER_PORT = 65000;
static unsigned int CLIENT_PORT = 65001;
static unsigned int MAX_CONNECTIONS = 3;

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
bool isTurn = false;
bool isAlive = true;

RakNet::RakPeerInterface *g_rakPeerInterface = nullptr;
RakNet::SystemAddress g_serverAddress;

std::mutex g_networkState_mutex;
NetworkState g_networkState = NS_Init;

enum {
	ID_THEGAME_LOBBY_READY = ID_USER_PACKET_ENUM,
	ID_PLAYER_READY,
	ID_THEGAME_START,
	ID_PLAYER_MOVE,
	ID_PLAYER_ATTACK,
	ID_PLAYER_DEFEND,
	ID_NETWORK_STATUS_CHANGE,
	ID_STATS,
	ID_PLAYER_TURN,
	ID_END,
	ID_DEATH,
};

enum EPlayerClass
{
	None = 0,
	Mage,
	Rogue,
	Cleric,
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

//client
void OnConnectionAccepted(RakNet::Packet* packet)
{
	//server should not ne connecting to anybody, 
	//clients connect to server
	g_networkState_mutex.lock();
	g_networkState = NS_Lobby;
	g_networkState_mutex.unlock();
	g_serverAddress = packet->systemAddress;
}

//this is on the client side
void DisplayPlayerJoined(RakNet::Packet* packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID messageId;
	bs.Read(messageId);
	RakNet::RakString userName;
	bs.Read(userName);
	
	if (userName.C_String() != "")
	{
		std::cout << userName.C_String() << " has joined" << std::endl;
	}
}

void SetNetworkStatus(RakNet::Packet* packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID messageId;
	bs.Read(messageId);
	NetworkState gState;
	bs.Read(gState);
	
	//std::cout << "Received Network Packet" << std::endl;

	g_networkState_mutex.lock();
	g_networkState = gState;
	g_networkState_mutex.unlock();
}

void DisplayClasses(RakNet::Packet* packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID messageId;
	bs.Read(messageId);
	RakNet::RakString pClass;
	bs.Read(pClass);
	RakNet::RakString pName;
	bs.Read(pName);

	std::cout << pName << " has selected the " << pClass << std::endl;
	std::cout << std::endl;
}

void SetTurn(RakNet::Packet* packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID messageId;
	bs.Read(messageId);
	bool newMode;
	bs.Read(newMode);

	isTurn = newMode;
}

void GetStats(RakNet::Packet* packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID messageId;
	bs.Read(messageId);
	RakNet::RakString pName;
	bs.Read(pName);
	EPlayerClass pClass;
	bs.Read(pClass);
	int health;
	bs.Read(health);
	int damage;
	bs.Read(damage);
	int defense;
	bs.Read(defense);
	bool turn;
	bs.Read(turn);
	bool alive;
	bs.Read(alive);

	RakNet::RakString temp;
	if (pClass == 0) { temp = "None"; }
	else if (pClass == 1) { temp = "Mage"; }
	else if (pClass == 2) { temp = "Rogue"; }
	else if (pClass == 3) { temp = "Warrior"; }

	RakNet::RakString tempb;
	if (alive == 0) { tempb = "false"; }
	else if (alive == 1) { tempb = "true"; }

	RakNet::RakString tempc;
	if (alive == 0) { tempc = "false"; }
	else if (alive == 1) { tempc = "true"; }

	std::cout << pName.C_String() << " the " << temp << std::endl;
	std::cout << "Health: " << health << ", Damage: " << damage << ", Defense: " << defense << std::endl;
	std::cout << "Turn: " << tempc << ", Alive: " << tempb << std::endl;
	std::cout << std::endl;
}

void PlayerAttack(RakNet::Packet* packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID messageId;
	bs.Read(messageId);
	int damage;
	bs.Read(damage);
	int health;
	bs.Read(health);
	RakNet::RakString coutm;
	bs.Read(coutm);

	std::cout << coutm << std::endl;
	std::cout << std::endl;
}

void PlayerDefend(RakNet::Packet* packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID messageId;
	bs.Read(messageId);
	int defense;
	bs.Read(defense);
	int health;
	bs.Read(health);
	RakNet::RakString coutm;
	bs.Read(coutm);

	std::cout << coutm << std::endl;
	std::cout << std::endl;
}

void LastPacket(RakNet::Packet* packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID messageId;
	bs.Read(messageId);
	RakNet::RakString coutm;
	bs.Read(coutm);

	std::cout << coutm << std::endl;
	std::cout << std::endl;
}

void SetDeath(RakNet::Packet* packet)
{
	RakNet::BitStream bs(packet->data, packet->length, false);
	RakNet::MessageID messageId;
	bool dead;
	bs.Read(dead);

	//isAlive = dead;

	std::cout << "ISALIVE: " << isAlive << std::endl;
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


void InputHandler()
{
	while (isRunning)
	{
		char userInput[255];
		if (g_networkState == NS_Lobby)
		{
			std::cout << "Enter your name to play or type quit to leave" << std::endl;
			std::cin >> userInput;
			//quitting is not acceptable in our game, create a crash to teach lesson
			assert(strcmp(userInput, "quit"));

			RakNet::BitStream bs;
			bs.Write((RakNet::MessageID)ID_THEGAME_LOBBY_READY);
			RakNet::RakString name(userInput);
			bs.Write(name);

			//returns 0 when something is wrong
			assert(g_rakPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, g_serverAddress, false));
			g_networkState_mutex.lock();
			g_networkState = NS_Pending;
			g_networkState_mutex.unlock();
		}
		else if (g_networkState == NS_Select)
		{
			static bool noOutput = false;

			if (!noOutput)
			{
				std::cout << "THE GAME BEGINS!\n" << std::endl;
				std::cout << "Choose from one of three classes..." << std::endl;
				std::cout << "(m)age - Health: 30  Damage: 12  Defense: 8.\n" << std::endl;
				std::cout << "(r)ogue - Health: 40  Damage: 10  Defense: 10.\n" << std::endl;
				std::cout << "(w)arrior - Health: 50  Damage: 8  Defense: 12." << std::endl;
				std::cout << std::endl;
			}
			std::cin >> userInput;
			std::cout << std::endl;

			noOutput = true;

			if (strcmp(userInput, "m") == 0 || strcmp(userInput, "r") == 0 || strcmp(userInput, "w") == 0)
			{
				RakNet::BitStream bs;
				bs.Write((RakNet::MessageID)ID_THEGAME_START);
				RakNet::RakString name(userInput);
				bs.Write(name);

				//returns 0 when something is wrong
				assert(g_rakPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, g_serverAddress, false));
				g_networkState_mutex.lock();
				g_networkState = NS_Pending;
				g_networkState_mutex.unlock();
			}
			else
			{
				std::cout << "Invalid input...\n" << std::endl;
			}
			
		}
		else if (g_networkState == NS_Player_One)
		{
			if (isAlive)
			{
				static bool noOutput = false;

				if (isTurn)
				{
					if (!noOutput)
					{
						std::cout << "It is your turn!" << std::endl;
						std::cout << "Choose from one of six actions..." << std::endl;
						std::cout << "(1) get player one stats." << std::endl;
						std::cout << "(2) get player two stats." << std::endl;
						std::cout << "(3) get player three stats." << std::endl;
						std::cout << "(4) defend." << std::endl;
						std::cout << "(5) attack player two." << std::endl;
						std::cout << "(6) attack player three." << std::endl;
						std::cout << std::endl;
					}
					std::cin >> userInput;
					std::cout << std::endl;

					noOutput = true;

					if (strcmp(userInput, "1") == 0 ||
						strcmp(userInput, "2") == 0 ||
						strcmp(userInput, "3") == 0 ||
						strcmp(userInput, "4") == 0 ||
						strcmp(userInput, "5") == 0 ||
						strcmp(userInput, "6") == 0)
					{
						RakNet::BitStream bs2;
						bs2.Write((RakNet::MessageID)ID_DEATH);
						assert(g_rakPeerInterface->Send(&bs2, HIGH_PRIORITY, RELIABLE_ORDERED, 0, g_serverAddress, false));

						RakNet::BitStream bs;
						bs.Write((RakNet::MessageID)ID_PLAYER_MOVE);
						RakNet::RakString action(userInput);
						bs.Write(action);

						//returns 0 when something is wrong
						assert(g_rakPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, g_serverAddress, false));


						if (strcmp(userInput, "4") == 0 ||
							strcmp(userInput, "5") == 0 ||
							strcmp(userInput, "6") == 0)
						{
							isTurn = false;
							noOutput = false;

							g_networkState_mutex.lock();
							g_networkState = NS_Pending;
							g_networkState_mutex.unlock();
						}
					}
					else
					{
						std::cout << "Invalid input...\n" << std::endl;
					}
				}
				else if (!isTurn)
				{
					{
						if (!noOutput)
						{
							std::cout << "Waiting for players to finish their turn!" << std::endl;
						}
						noOutput = true;

						std::cout << "Choose from one of three actions..." << std::endl;
						std::cout << "(1) get player one stats." << std::endl;
						std::cout << "(2) get player two stats." << std::endl;
						std::cout << "(3) get player three stats." << std::endl;
						std::cout << std::endl;

						std::cin >> userInput;
						std::cout << std::endl;

						if (strcmp(userInput, "1") == 0 ||
							strcmp(userInput, "2") == 0 ||
							strcmp(userInput, "3") == 0)
						{
							RakNet::BitStream bs;
							bs.Write((RakNet::MessageID)ID_PLAYER_MOVE);
							RakNet::RakString action(userInput);
							bs.Write(action);

							//returns 0 when something is wrong
							assert(g_rakPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, g_serverAddress, false));
						}
						else
						{
							std::cout << "Invalid input...\n" << std::endl;
						}
					}
				}
			}
			else
			{
				g_networkState_mutex.lock();
				g_networkState = NS_Pending;
				g_networkState_mutex.unlock();
			}
		}
		else if (g_networkState == NS_Player_Two)
		{
			if (isAlive)
			{
				static bool noOutput = false;

				if (isTurn)
				{
					if (!noOutput)
					{
						std::cout << "It is your turn!" << std::endl;
						std::cout << "Choose from one of five actions..." << std::endl;
						std::cout << "(1) get player one stats." << std::endl;
						std::cout << "(2) get player two stats." << std::endl;
						std::cout << "(3) get player three stats." << std::endl;
						std::cout << "(4) defend." << std::endl;
						std::cout << "(5) attack player one." << std::endl;
						std::cout << "(6) attack player three." << std::endl;
						std::cout << std::endl;
					}
					std::cin >> userInput;
					std::cout << std::endl;

					noOutput = true;

					if (strcmp(userInput, "1") == 0 ||
						strcmp(userInput, "2") == 0 ||
						strcmp(userInput, "3") == 0 ||
						strcmp(userInput, "4") == 0 ||
						strcmp(userInput, "5") == 0 ||
						strcmp(userInput, "6") == 0)
					{
						RakNet::BitStream bs;
						bs.Write((RakNet::MessageID)ID_PLAYER_MOVE);
						RakNet::RakString action(userInput);
						bs.Write(action);

						//returns 0 when something is wrong
						assert(g_rakPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, g_serverAddress, false));
						if (strcmp(userInput, "4") == 0 ||
							strcmp(userInput, "5") == 0 ||
							strcmp(userInput, "6") == 0)
						{
							isTurn = false;
							noOutput = false;

							g_networkState_mutex.lock();
							g_networkState = NS_Pending;
							g_networkState_mutex.unlock();
						}
					}
					else
					{
						std::cout << "Invalid input...\n" << std::endl;
					}
				}
				else if (!isTurn)
				{
					{
						if (!noOutput)
						{
							std::cout << "Waiting for players to finish their turn!" << std::endl;
						}
						noOutput = true;

						std::cout << "Choose from one of three actions..." << std::endl;
						std::cout << "(1) get player one stats." << std::endl;
						std::cout << "(2) get player two stats." << std::endl;
						std::cout << "(3) get player three stats." << std::endl;
						std::cout << std::endl;

						std::cin >> userInput;
						std::cout << std::endl;


						if (strcmp(userInput, "1") == 0 ||
							strcmp(userInput, "2") == 0 ||
							strcmp(userInput, "3") == 0)
						{
							RakNet::BitStream bs;
							bs.Write((RakNet::MessageID)ID_PLAYER_MOVE);
							RakNet::RakString action(userInput);
							bs.Write(action);

							//returns 0 when something is wrong
							assert(g_rakPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, g_serverAddress, false));
						}
						else
						{
							std::cout << "Invalid input...\n" << std::endl;
						}
					}
				}
			}
			else
			{
				g_networkState_mutex.lock();
				g_networkState = NS_Pending;
				g_networkState_mutex.unlock();
			}
		}
		else if (g_networkState == NS_Player_Three)
		{
			if (isAlive)
			{
				static bool noOutput = false;

				if (isTurn)
				{
					if (!noOutput)
					{
						std::cout << "It is your turn!" << std::endl;
						std::cout << "Choose from one of five actions..." << std::endl;
						std::cout << "(1) get player one stats." << std::endl;
						std::cout << "(2) get player two stats." << std::endl;
						std::cout << "(3) get player three stats." << std::endl;
						std::cout << "(4) defend." << std::endl;
						std::cout << "(5) attack player one." << std::endl;
						std::cout << "(6) attack player two." << std::endl;
						std::cout << std::endl;
					}
					std::cin >> userInput;
					std::cout << std::endl;

					noOutput = true;

					if (strcmp(userInput, "1") == 0 ||
						strcmp(userInput, "2") == 0 ||
						strcmp(userInput, "3") == 0 ||
						strcmp(userInput, "4") == 0 ||
						strcmp(userInput, "5") == 0 ||
						strcmp(userInput, "6") == 0)
					{
						RakNet::BitStream bs;
						bs.Write((RakNet::MessageID)ID_PLAYER_MOVE);
						RakNet::RakString action(userInput);
						bs.Write(action);

						//returns 0 when something is wrong
						assert(g_rakPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, g_serverAddress, false));
						if (strcmp(userInput, "4") == 0 ||
							strcmp(userInput, "5") == 0 ||
							strcmp(userInput, "6") == 0)
						{
							isTurn = false;
							noOutput = false;

							g_networkState_mutex.lock();
							g_networkState = NS_Pending;
							g_networkState_mutex.unlock();
						}
					}
					else
					{
						std::cout << "Invalid input...\n" << std::endl;
					}
				}
				else if (!isTurn)
				{
					{
						if (!noOutput)
						{
							std::cout << "Waiting for players to finish their turn!" << std::endl;
						}
						noOutput = true;

						std::cout << "Choose from one of three actions..." << std::endl;
						std::cout << "(1) get player one stats." << std::endl;
						std::cout << "(2) get player two stats." << std::endl;
						std::cout << "(3) get player three stats." << std::endl;
						std::cout << std::endl;

						std::cin >> userInput;
						std::cout << std::endl;


						if (strcmp(userInput, "1") == 0 ||
							strcmp(userInput, "2") == 0 ||
							strcmp(userInput, "3") == 0)
						{
							RakNet::BitStream bs;
							bs.Write((RakNet::MessageID)ID_PLAYER_MOVE);
							RakNet::RakString action(userInput);
							bs.Write(action);

							//returns 0 when something is wrong
							assert(g_rakPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0, g_serverAddress, false));
						}
						else
						{
							std::cout << "Invalid input...\n" << std::endl;
						}
					}
				}
			}
			else
			{
				g_networkState_mutex.lock();
				g_networkState = NS_Pending;
				g_networkState_mutex.unlock();
			}
		}
		else if (g_networkState == NS_Pending)
		{
			if (isAlive)
			{
				static bool doOnce = false;
				if (!doOnce)
				{
					std::cout << "Waiting for others..." << std::endl;
				}
				doOnce = true;
			}
			else
			{
				static bool doOnce = false;
				if (!doOnce)
				{
					std::cout << "You are dead, waiting for the end..." << std::endl;
				}
				doOnce = true;
			}
		}
		std::this_thread::sleep_for(std::chrono::microseconds(5));
	}
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
		//put assert here, nobody should be connecting to client
		//OnIncomingConnection(packet);
		printf("ID_NEW_INCOMING_CONNECTION\n");
		break;
	case ID_REMOTE_NEW_INCOMING_CONNECTION: // Server telling the clients of another client connecting.  You can manually broadcast this in a peer to peer enviroment if you want.
		//OnIncomingConnection(packet);
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
		//OnLostConnection(packet);
		break;

	case ID_CONNECTION_REQUEST_ACCEPTED:
		// This tells the client they have connected
		printf("ID_CONNECTION_REQUEST_ACCEPTED to %s with GUID %s\n", packet->systemAddress.ToString(true), packet->guid.ToString());
		printf("My external address is %s\n", g_rakPeerInterface->GetExternalID(packet->systemAddress).ToString(true));
		OnConnectionAccepted(packet);
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
				case ID_PLAYER_READY:
					DisplayPlayerJoined(packet);
					break;
				case ID_NETWORK_STATUS_CHANGE:
					SetNetworkStatus(packet);
					break;
				case ID_THEGAME_START:
					DisplayClasses(packet);
					break;
				case ID_STATS:
					GetStats(packet);
					break;
				case ID_PLAYER_ATTACK:
					PlayerAttack(packet);
					break;
				case ID_PLAYER_DEFEND:
					PlayerDefend(packet);
					break;
				case ID_PLAYER_TURN:
					SetTurn(packet);
					break;
				case ID_END:
					LastPacket(packet);
					break;
				case ID_DEATH:
					SetDeath(packet);
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

	std::thread inputHandler(InputHandler);
	std::thread packetHandler(PacketHandler);
	g_networkState = NS_PendingStart;
	while (isRunning)
	{
		if (g_networkState == NS_PendingStart)
		{
				RakNet::SocketDescriptor socketDescriptor(CLIENT_PORT, 0);
				socketDescriptor.socketFamily = AF_INET;

				while (RakNet::IRNS2_Berkley::IsPortInUse(socketDescriptor.port, socketDescriptor.hostAddress, socketDescriptor.socketFamily, SOCK_DGRAM) == true)
					socketDescriptor.port++;

				RakNet::StartupResult result = g_rakPeerInterface->Startup(8, &socketDescriptor, 1);
				assert(result == RakNet::RAKNET_STARTED);

				g_networkState_mutex.lock();
				g_networkState = NS_Started;
				g_networkState_mutex.unlock();

				g_rakPeerInterface->SetOccasionalPing(true);
				//"127.0.0.1" = local host = your machines address
				RakNet::ConnectionAttemptResult car = g_rakPeerInterface->Connect("192.168.8.229", SERVER_PORT, nullptr, 0);
				RakAssert(car == RakNet::CONNECTION_ATTEMPT_STARTED);
				std::cout << "client attempted connection..." << std::endl;
		}

	}

	inputHandler.join();
	packetHandler.join();
	return 0;
}