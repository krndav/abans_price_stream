#include <bits/stdc++.h>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_PORT 3000
#define SERVER_IP "127.0.0.1"  
#define PACKET_SIZE 17

struct Packet {
    std::string symbol;
    char bs_indicator;
    int32_t quantity;
    int32_t price;
    int32_t seq;

    void print() const {
        std::cout << "Symbol: " << symbol
                  << ", Type: " << bs_indicator
                  << ", Qty: " << quantity
                  << ", Price: " << price
                  << ", Seq: " << seq << std::endl;
    }
};

int connectToServer() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    sockaddr_in serv_addr{};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
        perror("Connection Failed");
        exit(EXIT_FAILURE);
    }

    return sock;
}

void sendStreamAllPacketsRequest(int sock) {
    uint8_t payload[2] = {1, 0};
    send(sock, (const char*)payload, sizeof(payload), 0);
}

Packet parsePacket(const uint8_t* buffer) {
    Packet pkt;

    pkt.symbol = std::string(reinterpret_cast<const char*>(buffer), 4);
    pkt.bs_indicator = buffer[4];
    pkt.quantity = ntohl(*reinterpret_cast<const int32_t*>(buffer + 5));
    pkt.price = ntohl(*reinterpret_cast<const int32_t*>(buffer + 9));
    pkt.seq = ntohl(*reinterpret_cast<const int32_t*>(buffer + 13));

    return pkt;
}

std::map<int, Packet> receivePackets(int sock, std::set<int>& seenSeqs, int& maxSeq) {
    std::map<int, Packet> packets;
    uint8_t buffer[PACKET_SIZE];

    while (true) {
        ssize_t bytes = recv(sock, (char*)buffer, PACKET_SIZE, MSG_WAITALL);
        if (bytes == 0) break;
        if (bytes != PACKET_SIZE) {
            std::cerr << "Incomplete packet received!" << std::endl;
            continue;
        }

        Packet pkt = parsePacket(buffer);
        seenSeqs.insert(pkt.seq);
        if (pkt.seq > maxSeq) maxSeq = pkt.seq;
        packets[pkt.seq] = pkt;
    }

    closesocket(sock);
    return packets;
}

Packet requestMissingPacket(int seq) {
    int sock = connectToServer();

    uint8_t payload[2] = {2, static_cast<uint8_t>(seq)};
    send(sock, (const char*)payload, sizeof(payload), 0);

    uint8_t buffer[PACKET_SIZE];
    ssize_t bytes = recv(sock, (char*)buffer, PACKET_SIZE, MSG_WAITALL);
    if (bytes != PACKET_SIZE) {
        std::cerr << "Error receiving packet for seq " << seq << std::endl;
    }

    Packet pkt = parsePacket(buffer);
    closesocket(sock);
    return pkt;
}
 int solve(){
     WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed!" << std::endl;
        return 1;
    }

    std::set<int> seenSeqs;
    int maxSeq = 0;

    int sock = connectToServer();
    sendStreamAllPacketsRequest(sock);
    std::map<int, Packet> packets = receivePackets(sock, seenSeqs, maxSeq);

    for (int seq = 1; seq <= maxSeq; ++seq) {
        if (!seenSeqs.count(seq)) {
            Packet missed = requestMissingPacket(seq);
            packets[seq] = missed;
        }
    }

    for (int seq = 1; seq <= maxSeq; ++seq) {
        packets[seq].print();
    }

    WSACleanup();
    return 1;

 }

int main() {
    solve();
    return 0;
}
