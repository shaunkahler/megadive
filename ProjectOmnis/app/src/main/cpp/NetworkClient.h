#pragma once

#include <string>
#include <vector>
#include "ECS.h"

class NetworkClient {
public:
    NetworkClient();
    ~NetworkClient();

    void Initialize(const std::string& serverIp, int port);
    void Update(); // Called on background thread (Core 2)

    // Multiplayer MMO Sync [MQ-5]
    void SendPlayerPosition(const float pos[3], const float rot[4]);
    void SendWorldModification(const EntityModificationPacket& packet);

private:
    std::string m_serverIp;
    int m_port;
    bool m_connected;

    // UDP Socket structures (Stubbed)
    // int m_socket; 

    void ProcessIncomingPackets();
};
