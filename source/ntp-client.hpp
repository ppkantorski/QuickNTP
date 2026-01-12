#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <string>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define UNIX_OFFSET 2208988800L
#define NTP_DEFAULT_SERVER "pool.ntp.org"
#define NTP_DEFAULT_PORT "123"
#define NTP_DEFAULT_TIMEOUT 3
#define NTP_FLAGS 0x23

typedef struct {
    uint8_t flags;
    uint8_t stratum;
    uint8_t poll;
    uint8_t precision;
    uint32_t root_delay;
    uint32_t root_dispersion;
    uint8_t referenceID[4];
    uint32_t ref_ts_secs;
    uint32_t ref_ts_frac;
    uint32_t origin_ts_secs;
    uint32_t origin_ts_frac;
    uint32_t recv_ts_secs;
    uint32_t recv_ts_fracs;
    uint32_t transmit_ts_secs;
    uint32_t transmit_ts_frac;
} ntp_packet;

class NTPClient {
private:
    const char* m_server;
    const char* m_port;
    int m_timeout;

public:
    NTPClient(const char* server = NTP_DEFAULT_SERVER, const char* port = NTP_DEFAULT_PORT, int timeout = NTP_DEFAULT_TIMEOUT)
        : m_server(server), m_port(port), m_timeout(timeout) {}

    time_t getTime() {
        int status;
        struct addrinfo hints, *servinfo;
        hints = (struct addrinfo){.ai_family = AF_INET, .ai_socktype = SOCK_DGRAM};
        
        if ((status = getaddrinfo(m_server, m_port, &hints, &servinfo)) != 0) {
            return 0; // Return 0 on error
        }

        struct addrinfo* ap;
        int server_sock;
        ntp_packet packet = {.flags = NTP_FLAGS};
        bool time_retrieved = false;

        for (ap = servinfo; ap != nullptr; ap = ap->ai_next) {
            server_sock = socket(ap->ai_family, ap->ai_socktype, ap->ai_protocol);
            if (server_sock == -1) {
                continue;
            }

            struct timeval tv = {.tv_sec = m_timeout, .tv_usec = 0};
            if (setsockopt(server_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
                close(server_sock);
                freeaddrinfo(servinfo);
                return 0; // Return 0 on error
            }

            if (setsockopt(server_sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) {
                close(server_sock);
                freeaddrinfo(servinfo);
                return 0; // Return 0 on error
            }

            if (sendto(server_sock, &packet, sizeof(packet), 0, ap->ai_addr, ap->ai_addrlen) == -1) {
                close(server_sock);
                continue;
            }

            struct sockaddr_storage server_addr;
            socklen_t server_addr_len = sizeof(server_addr);
            if (recvfrom(server_sock, &packet, sizeof(packet), 0, (struct sockaddr*)&server_addr, &server_addr_len) == -1) {
                close(server_sock);
                continue;
            }

            time_retrieved = true;
            break;
        }

        freeaddrinfo(servinfo);

        if (!time_retrieved) {
            return 0; // Return 0 on error
        }

        close(server_sock);

        packet.recv_ts_secs = ntohl(packet.recv_ts_secs);
        return packet.recv_ts_secs - UNIX_OFFSET;
    }

    long getTimeOffset(time_t currentTime) {
        time_t ntpTime = getTime();
        if (ntpTime == 0) {
            return LLONG_MIN; // Return special value on error
        }
        return currentTime - ntpTime;
    }
};