
#include <xcopy.h>
#include <udpcopy.h>

inline uint64_t get_key(uint32_t ip, uint16_t port)
{
    uint64_t value = ((uint64_t)ip) << 16;
    value += port;
    return value;
}

inline uint16_t get_appropriate_port(uint16_t orig_port, uint16_t add)
{
    uint16_t dest_port = orig_port;
    if (dest_port < (65536 - add)){
        dest_port += add;
    }else{
        dest_port  = 1024 + add;
    }
    return dest_port;
}

static unsigned int seed = 0;

uint16_t get_port_by_rand_addition(uint16_t orig_port)
{
    struct timeval  tp;
    uint16_t        port_add;

    if (0 == seed){    
        gettimeofday(&tp, NULL);
        seed = tp.tv_usec;
    }    
    port_add = (uint16_t)(4096*(rand_r(&seed)/(RAND_MAX + 1.0)));
    port_add = port_add + 32768;

    return get_appropriate_port(ntohs(orig_port), port_add);
}

uint16_t get_port_from_shift(uint16_t orig_port, uint16_t rand_port,
        int shift_factor)
{
    uint16_t        port_add;
    port_add = (shift_factor << 11) + rand_port;

    return get_appropriate_port(ntohs(orig_port), port_add);
}

ip_port_pair_mapping_t *get_test_pair(ip_port_pair_mappings_t *transfer,
        uint32_t ip, uint16_t port)
{
    int i;
    ip_port_pair_mapping_t  *pair = NULL;
    ip_port_pair_mapping_t **mappings;

    mappings = transfer->mappings;
    for (i = 0; i < transfer->num; i++){
        pair = mappings[i];
        if (ip == pair->target_ip && port == pair->target_port){
            break;
        }
    }

    return pair;
}

int check_pack_src(ip_port_pair_mappings_t *transfer,
        uint32_t ip, uint16_t port, int src_flag)
{
    int i;
    int ret = UNKNOWN;
    ip_port_pair_mapping_t *pair;
    ip_port_pair_mapping_t **mappings = transfer->mappings;
    for (i = 0; i < transfer->num; i++){
        pair = mappings[i];
        if (CHECK_DEST == src_flag){
            /* We are interested in INPUT raw socket */
            if (ip == pair->online_ip && port == pair->online_port){
                ret = LOCAL;
                break;
            }else if (0 == pair->online_ip && port == pair->online_port){
                ret = LOCAL;
                break;
            }
        }else if (CHECK_SRC == src_flag){
            if (ip == pair->target_ip && port == pair->target_port){
                ret = REMOTE;
                break;
            }
        }
    }
    return ret;
}

unsigned char *copy_ip_packet(struct iphdr *ip_header)
{
    uint16_t tot_len    = ntohs(ip_header->tot_len);
    unsigned char *data = (unsigned char *)malloc(tot_len);
    if (NULL != data){    
        memcpy(data, ip_header, tot_len);
    }    
    return data;
}

unsigned short csum(unsigned short *packet, int pack_len) 
{ 
    register unsigned long sum = 0; 
    while (pack_len > 1) {
        sum += *(packet++); 
        pack_len -= 2; 
    } 
    if (pack_len > 0) {
        sum += *(unsigned char *)packet; 
    }
    while (sum >> 16){
        sum = (sum & 0xffff) + (sum >> 16); 
    }
    return (unsigned short) ~sum; 
} 


static int
do_checksum_math(u_int16_t *data, int len)
{   
    int sum = 0;
    union {
        u_int16_t s;
        u_int8_t b[2];
    } pad;

    while (len > 1) {
        sum += *data++;
        len -= 2;
    }

    if (len == 1) {
        pad.b[0] = *(u_int8_t *)data;
        pad.b[1] = 0;
        sum += pad.s;
    }

    return (sum);
} 

void udpcsum(struct iphdr *ip_header, struct udphdr *udp_packet)
{       
    int            sum;
    uint16_t       len;

    udp_packet->check = 0;

    len  = ntohs(udp_packet->len);
    sum  = do_checksum_math((u_int16_t *)&ip_header->saddr, 8); 
    sum += ntohs(IPPROTO_UDP + len);
    sum += do_checksum_math((u_int16_t *)udp_packet, len);
    udp_packet->check = CHECKSUM_CARRY(sum);

}  


