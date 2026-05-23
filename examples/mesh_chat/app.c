#include "../../Pala_One_2_1/pala_app.h"
#include "../../Pala_One_2_1/pala_api.h"

__attribute__((section(".header")))
const PalaAppHeader pala_header = {
    .magic        = PALA_APP_MAGIC,
    .api_version  = PALA_API_VERSION,
    .name         = "Mesh Chat",
    .entry_offset = 0,
    .reloc_offset = 0,
    .reloc_count  = 0,
};

// ── Meshtastic LongFast radio parameters ─────────────────────────────────────
// Adjust MESH_FREQ_MHZ for your region:
//   EU 868: 869.4875   US 915: 906.875   AU 915: 916.875   IN 865: 865.2125
#define MESH_FREQ_MHZ   869.4875f
#define MESH_BW_KHZ     250.0f
#define MESH_SF         11
#define MESH_CR         5
#define MESH_SYNC_WORD  0x2B
#define MESH_TX_POWER   22
#define MESH_PREAMBLE   16
#define MESH_TCXO_V     1.8f

// ── Meshtastic LongFast channel key (PSK "AQ==" = 0x01, padded to 16 bytes)
// Verify against src/mesh/Channel.cpp defaultpsk[] in Meshtastic firmware.
static const uint8_t MESH_KEY[16] = {
    0xd4,0xf1,0xbb,0x3a,0x20,0x29,0x07,0x59,
    0xf0,0xbc,0xff,0xab,0xcf,0x4e,0x69,0x01
};
static const uint8_t MESH_CHANNEL_HASH = 0x08;
#define MESH_HOP_LIMIT  3
#define MESH_TEXT_MAX   80

// ── App constants
#define MAX_INBOX    10
#define NUM_QUICK    5
#define LONG_MS      850

// 2D array avoids pointer relocations in PIC binary
static const char QUICK_MSGS[NUM_QUICK][32] = {
    "On my way",
    "Need help, please respond",
    "OK / Acknowledged",
    "Testing 1-2-3",
    "Ping",
};

// ── Inbox storage
typedef struct {
    uint32_t from;
    uint32_t rx_time;
    char     text[MESH_TEXT_MAX + 1];
} MeshMsg;

typedef struct {
    uint8_t count;
    MeshMsg msgs[MAX_INBOX];
} Inbox;

#define VIEW_INBOX  0
#define VIEW_SEND   1

// ────────────────────────────────────────────────────────────────────────────
// AES-128 (pure C, no external dependencies)
// ────────────────────────────────────────────────────────────────────────────

static const uint8_t AES_SBOX[256] = {
    0x63,0x7c,0x77,0x7b,0xf2,0x6b,0x6f,0xc5,0x30,0x01,0x67,0x2b,0xfe,0xd7,0xab,0x76,
    0xca,0x82,0xc9,0x7d,0xfa,0x59,0x47,0xf0,0xad,0xd4,0xa2,0xaf,0x9c,0xa4,0x72,0xc0,
    0xb7,0xfd,0x93,0x26,0x36,0x3f,0xf7,0xcc,0x34,0xa5,0xe5,0xf1,0x71,0xd8,0x31,0x15,
    0x04,0xc7,0x23,0xc3,0x18,0x96,0x05,0x9a,0x07,0x12,0x80,0xe2,0xeb,0x27,0xb2,0x75,
    0x09,0x83,0x2c,0x1a,0x1b,0x6e,0x5a,0xa0,0x52,0x3b,0xd6,0xb3,0x29,0xe3,0x2f,0x84,
    0x53,0xd1,0x00,0xed,0x20,0xfc,0xb1,0x5b,0x6a,0xcb,0xbe,0x39,0x4a,0x4c,0x58,0xcf,
    0xd0,0xef,0xaa,0xfb,0x43,0x4d,0x33,0x85,0x45,0xf9,0x02,0x7f,0x50,0x3c,0x9f,0xa8,
    0x51,0xa3,0x40,0x8f,0x92,0x9d,0x38,0xf5,0xbc,0xb6,0xda,0x21,0x10,0xff,0xf3,0xd2,
    0xcd,0x0c,0x13,0xec,0x5f,0x97,0x44,0x17,0xc4,0xa7,0x7e,0x3d,0x64,0x5d,0x19,0x73,
    0x60,0x81,0x4f,0xdc,0x22,0x2a,0x90,0x88,0x46,0xee,0xb8,0x14,0xde,0x5e,0x0b,0xdb,
    0xe0,0x32,0x3a,0x0a,0x49,0x06,0x24,0x5c,0xc2,0xd3,0xac,0x62,0x91,0x95,0xe4,0x79,
    0xe7,0xc8,0x37,0x6d,0x8d,0xd5,0x4e,0xa9,0x6c,0x56,0xf4,0xea,0x65,0x7a,0xae,0x08,
    0xba,0x78,0x25,0x2e,0x1c,0xa6,0xb4,0xc6,0xe8,0xdd,0x74,0x1f,0x4b,0xbd,0x8b,0x8a,
    0x70,0x3e,0xb5,0x66,0x48,0x03,0xf6,0x0e,0x61,0x35,0x57,0xb9,0x86,0xc1,0x1d,0x9e,
    0xe1,0xf8,0x98,0x11,0x69,0xd9,0x8e,0x94,0x9b,0x1e,0x87,0xe9,0xce,0x55,0x28,0xdf,
    0x8c,0xa1,0x89,0x0d,0xbf,0xe6,0x42,0x68,0x41,0x99,0x2d,0x0f,0xb0,0x54,0xbb,0x16,
};

static const uint8_t AES_RCON[10] = {
    0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x1b,0x36
};

static uint8_t aes_xtime(uint8_t a) {
    return (uint8_t)((a << 1) ^ ((a & 0x80) ? 0x1b : 0));
}

/* AES state is column-major: byte at row r, col c is s[c*4+r] */

static void aes_sub_bytes(uint8_t s[16]) {
    int i;
    for (i = 0; i < 16; i++) s[i] = AES_SBOX[s[i]];
}

static void aes_shift_rows(uint8_t s[16]) {
    uint8_t t;
    t=s[1];  s[1]=s[5];  s[5]=s[9];  s[9]=s[13]; s[13]=t;
    t=s[2];  s[2]=s[10]; s[10]=t;    t=s[6]; s[6]=s[14]; s[14]=t;
    t=s[15]; s[15]=s[11]; s[11]=s[7]; s[7]=s[3]; s[3]=t;
}

static void aes_mix_columns(uint8_t s[16]) {
    int c;
    for (c = 0; c < 4; c++) {
        uint8_t a0=s[c*4], a1=s[c*4+1], a2=s[c*4+2], a3=s[c*4+3];
        uint8_t x = a0^a1^a2^a3;
        s[c*4+0] = (uint8_t)(a0 ^ x ^ aes_xtime(a0^a1));
        s[c*4+1] = (uint8_t)(a1 ^ x ^ aes_xtime(a1^a2));
        s[c*4+2] = (uint8_t)(a2 ^ x ^ aes_xtime(a2^a3));
        s[c*4+3] = (uint8_t)(a3 ^ x ^ aes_xtime(a3^a0));
    }
}

static void aes_key_expand(const uint8_t key[16], uint8_t rk[11][16]) {
    int i, j;
    for (i = 0; i < 16; i++) rk[0][i] = key[i];
    for (i = 1; i <= 10; i++) {
        uint8_t t0=rk[i-1][12], t1=rk[i-1][13], t2=rk[i-1][14], t3=rk[i-1][15];
        /* RotWord + SubWord */
        uint8_t s0=AES_SBOX[t1], s1=AES_SBOX[t2], s2=AES_SBOX[t3], s3=AES_SBOX[t0];
        s0 ^= AES_RCON[i-1];
        rk[i][0]=(uint8_t)(rk[i-1][0]^s0); rk[i][1]=(uint8_t)(rk[i-1][1]^s1);
        rk[i][2]=(uint8_t)(rk[i-1][2]^s2); rk[i][3]=(uint8_t)(rk[i-1][3]^s3);
        for (j = 4; j < 16; j++) rk[i][j] = (uint8_t)(rk[i-1][j] ^ rk[i][j-4]);
    }
}

static void aes128_encrypt_block(const uint8_t rk[11][16], const uint8_t in[16], uint8_t out[16]) {
    uint8_t s[16];
    int i, r;
    for (i = 0; i < 16; i++) s[i] = (uint8_t)(in[i] ^ rk[0][i]);
    for (r = 1; r <= 10; r++) {
        aes_sub_bytes(s);
        aes_shift_rows(s);
        if (r < 10) aes_mix_columns(s);
        for (i = 0; i < 16; i++) s[i] ^= rk[r][i];
    }
    for (i = 0; i < 16; i++) out[i] = s[i];
}

/* AES-128-CTR — nonce incremented big-endian to match mbedTLS ctr behaviour */
static void aes128_ctr(const uint8_t rk[11][16], const uint8_t nonce[16],
                       const uint8_t* in, int len, uint8_t* out) {
    uint8_t counter[16], ks[16];
    int i, j, pos = 0;
    for (i = 0; i < 16; i++) counter[i] = nonce[i];
    while (pos < len) {
        aes128_encrypt_block(rk, counter, ks);
        for (j = 15; j >= 0; j--) { counter[j]++; if (counter[j]) break; }
        for (j = 0; j < 16 && pos < len; j++, pos++)
            out[pos] = (uint8_t)(in[pos] ^ ks[j]);
    }
}

// ── Meshtastic helpers ────────────────────────────────────────────────────────

/* Nonce layout: packet_id (4B LE, zero-padded to 8B) + from_node (4B LE, zero-padded to 8B) */
static void meshCrypt(const uint8_t rk[11][16], uint32_t packet_id, uint32_t from_node,
                      const uint8_t* in, int len, uint8_t* out) {
    uint8_t nonce[16] = {0};
    nonce[0]=(uint8_t)(packet_id);      nonce[1]=(uint8_t)(packet_id>>8);
    nonce[2]=(uint8_t)(packet_id>>16);  nonce[3]=(uint8_t)(packet_id>>24);
    nonce[8]=(uint8_t)(from_node);      nonce[9]=(uint8_t)(from_node>>8);
    nonce[10]=(uint8_t)(from_node>>16); nonce[11]=(uint8_t)(from_node>>24);
    aes128_ctr(rk, nonce, in, len, out);
}

/* Meshtastic 16-byte packet header (little-endian, packed) */
typedef struct __attribute__((packed)) {
    uint32_t to;
    uint32_t from;
    uint32_t id;
    uint8_t  flags;        /* bits 5-7: hop_start | bits 0-2: hop_limit */
    uint8_t  channel_hash;
    uint8_t  reserved[2];
} MeshHdr;

/* Hand-crafted protobuf: Data{portnum=1(TEXT_MESSAGE_APP), payload=text} */
static int meshEncodeText(const char* text, uint8_t* buf, int bufMax) {
    int i, tlen = 0;
    while (text[tlen]) tlen++;
    if (tlen > 255 || 4 + tlen > bufMax) return 0;
    int pos = 0;
    buf[pos++] = 0x08; buf[pos++] = 0x01;   /* field 1 (portnum) = 1 */
    buf[pos++] = 0x12; buf[pos++] = (uint8_t)tlen;
    for (i = 0; i < tlen; i++) buf[pos++] = (uint8_t)text[i];
    return pos;
}

/* Returns 1 if portnum==1 (TEXT_MESSAGE_APP) and fills textOut */
static int meshDecodeText(const uint8_t* buf, int len, char* textOut, int textMax) {
    int portnum = 0, gotText = 0, pos = 0;
    while (pos < len) {
        uint8_t tag_wire = buf[pos++];
        int field = tag_wire >> 3;
        int wire  = tag_wire & 0x07;
        if (wire == 0) {
            uint32_t val = 0; int sh = 0;
            while (pos < len) {
                uint8_t b = buf[pos++];
                val |= (uint32_t)((b & 0x7F) << sh);
                if (!(b & 0x80)) break;
                sh += 7;
            }
            if (field == 1) portnum = (int)val;
        } else if (wire == 2) {
            uint32_t slen = 0; int sh = 0;
            while (pos < len) {
                uint8_t b = buf[pos++];
                slen |= (uint32_t)((b & 0x7F) << sh);
                if (!(b & 0x80)) break;
                sh += 7;
            }
            if (field == 2 && portnum == 1 && pos + (int)slen <= len) {
                int copy = (int)slen < textMax - 1 ? (int)slen : textMax - 1;
                int i; for (i = 0; i < copy; i++) textOut[i] = (char)buf[pos + i];
                textOut[copy] = '\0';
                gotText = 1;
            }
            pos += (int)slen;
        } else {
            break;
        }
    }
    return gotText && portnum == 1;
}

/* Assemble a Meshtastic broadcast packet into out[]; returns length or 0 on error */
static int meshBuildPacket(const uint8_t rk[11][16], uint32_t nodeId, uint32_t nowMs,
                           const char* text, uint8_t* out, int outMax) {
    uint8_t plain[130];
    int plen, i;

    plen = meshEncodeText(text, plain, (int)sizeof(plain));
    if (plen <= 0 || 16 + plen > outMax) return 0;

    /* Pseudo-random packet ID: mix node ID with millisecond timestamp */
    uint32_t packetId = nodeId ^ nowMs ^ (nodeId >> 16);

    MeshHdr hdr;
    hdr.to           = 0xFFFFFFFF;
    hdr.from         = nodeId;
    hdr.id           = packetId;
    hdr.flags        = (uint8_t)(((MESH_HOP_LIMIT & 0x07) << 5) | (MESH_HOP_LIMIT & 0x07));
    hdr.channel_hash = MESH_CHANNEL_HASH;
    hdr.reserved[0]  = 0;
    hdr.reserved[1]  = 0;

    for (i = 0; i < 16; i++) out[i] = ((const uint8_t*)&hdr)[i];
    meshCrypt(rk, packetId, nodeId, plain, plen, out + 16);
    return 16 + plen;
}

/* Decrypt and decode an incoming raw packet; returns 1 on success */
static int meshParsePacket(const uint8_t rk[11][16],
                           const uint8_t* raw, int rawLen,
                           uint32_t* fromOut, char* textOut, int textMax) {
    uint8_t plain[220];
    int payLen;
    if (rawLen < 20) return 0;
    const MeshHdr* hdr = (const MeshHdr*)raw;
    payLen = rawLen - 16;
    if (payLen > (int)sizeof(plain)) return 0;
    meshCrypt(rk, hdr->id, hdr->from, raw + 16, payLen, plain);
    if (!meshDecodeText(plain, payLen, textOut, textMax)) return 0;
    *fromOut = hdr->from;
    return 1;
}

// ── UI helpers ────────────────────────────────────────────────────────────────

/* Provided so the linker has no undefined PLT symbols; -nostdlib omits libc */
void* memcpy(void* dst, const void* src, unsigned n) {
    unsigned char* d = (unsigned char*)dst;
    const unsigned char* s = (const unsigned char*)src;
    while (n--) *d++ = *s++;
    return dst;
}

static int slen(const char* s) {
    int n = 0; while (s[n]) n++; return n;
}

static void formatAge(const PalaAPI* api, uint32_t rx_time, char* buf, int buflen) {
    uint32_t now = api->rtcSeconds();
    uint32_t age = (now >= rx_time) ? (now - rx_time) : 0;
    if (age < 60)
        api->snprintf_wrap(buf, buflen, "%us ago", (unsigned)age);
    else if (age < 3600)
        api->snprintf_wrap(buf, buflen, "%um ago", (unsigned)(age / 60));
    else
        api->snprintf_wrap(buf, buflen, "%uh ago", (unsigned)(age / 3600));
}

static void drawInbox(const PalaAPI* api, const Inbox* inbox, int cur) {
    char buf[48];
    char line1[36], line2[36];
    int len, rem, i;

    api->clearScreen();
    api->drawHeader("Mesh Inbox");

    if (inbox->count == 0) {
        api->drawTextAt(45, 55, "No messages yet", 0);
        api->drawTextAt(95, 112, "dbl:send  hold:exit", 0);
    } else {
        const MeshMsg* m = &inbox->msgs[cur];

        char age[16];
        formatAge(api, m->rx_time, age, sizeof(age));
        api->snprintf_wrap(buf, sizeof(buf), "!%08lx  %s", (unsigned long)m->from, age);
        api->drawTextAt(3, 24, buf, 0);

        len = slen(m->text);
        if (len <= 35) {
            for (i = 0; i < len; i++) line1[i] = m->text[i];
            line1[len] = '\0';
            line2[0] = '\0';
        } else {
            for (i = 0; i < 35; i++) line1[i] = m->text[i];
            line1[35] = '\0';
            rem = len - 35;
            if (rem > 35) rem = 35;
            for (i = 0; i < rem; i++) line2[i] = m->text[35 + i];
            line2[rem] = '\0';
        }
        api->drawTextAt(3, 44, line1, 1);
        if (line2[0]) api->drawTextAt(3, 58, line2, 1);

        api->snprintf_wrap(buf, sizeof(buf), "[%d/%d]", cur + 1, (int)inbox->count);
        api->drawTextAt(3, 112, buf, 0);
        api->drawTextAt(95, 112, "dbl:send  hold:exit", 0);
    }

    api->refreshDisplay();
}

static void drawSend(const PalaAPI* api, int sel) {
    int i;
    api->clearScreen();
    api->drawHeader("Mesh Send");

    for (i = 0; i < NUM_QUICK; i++) {
        int y = 24 + i * 14;
        if (i == sel) {
            api->drawTextAt(3,  y, ">", 1);
            api->drawTextAt(14, y, QUICK_MSGS[i], 1);
        } else {
            api->drawTextAt(14, y, QUICK_MSGS[i], 0);
        }
    }

    api->drawTextAt(3, 112, "hold:send  dbl:inbox", 0);
    api->refreshDisplay();
}

static void drawSending(const PalaAPI* api, const char* text) {
    api->clearScreen();
    api->drawHeader("Mesh Send");
    api->drawTextAt(50, 48, "Sending...", 1);
    api->drawTextAt(3,  68, text, 0);
    api->refreshDisplay();
}

// ── App entry point ───────────────────────────────────────────────────────────

void app_main(const PalaAPI* api) {
    uint8_t rk[11][16];
    Inbox   inbox;
    uint8_t rawbuf[250];
    int     view        = VIEW_INBOX;
    int     cur_msg     = 0;
    int     cur_send    = 0;
    int     needsRedraw = 1;
    uint32_t pressStart = 0;
    int      long_fired = 0;

    api->loraInit(MESH_FREQ_MHZ, MESH_BW_KHZ, MESH_SF, MESH_CR,
                  MESH_SYNC_WORD, MESH_TX_POWER, MESH_PREAMBLE, MESH_TCXO_V);
    aes_key_expand(MESH_KEY, rk);

    inbox.count = 0;
    if (api->storageRead("mesh_inbox", &inbox, sizeof(inbox)) != (int)sizeof(inbox))
        inbox.count = 0;
    if (inbox.count > MAX_INBOX) inbox.count = MAX_INBOX;

    while (1) {
        uint32_t now    = api->millisNow();
        uint32_t nodeId = api->loraNodeId();

        /* Poll for incoming LoRa packets */
        {
            int rxLen = api->loraRecv(rawbuf, (int)sizeof(rawbuf));
            if (rxLen > 0) {
                uint32_t from = 0;
                char text[MESH_TEXT_MAX + 1];
                if (meshParsePacket((const uint8_t(*)[16])rk, rawbuf, rxLen,
                                    &from, text, (int)sizeof(text))) {
                    if (inbox.count < MAX_INBOX) {
                        int idx = inbox.count;
                        inbox.msgs[idx].from    = from;
                        inbox.msgs[idx].rx_time = api->rtcSeconds();
                        int i, tlen = slen(text);
                        if (tlen > MESH_TEXT_MAX) tlen = MESH_TEXT_MAX;
                        for (i = 0; i < tlen; i++) inbox.msgs[idx].text[i] = text[i];
                        inbox.msgs[idx].text[tlen] = '\0';
                        inbox.count++;
                    } else {
                        /* Drop oldest, shift remaining */
                        int i;
                        for (i = 0; i < MAX_INBOX - 1; i++)
                            inbox.msgs[i] = inbox.msgs[i + 1];
                        inbox.msgs[MAX_INBOX - 1].from    = from;
                        inbox.msgs[MAX_INBOX - 1].rx_time = api->rtcSeconds();
                        int tlen = slen(text);
                        if (tlen > MESH_TEXT_MAX) tlen = MESH_TEXT_MAX;
                        for (i = 0; i < tlen; i++)
                            inbox.msgs[MAX_INBOX - 1].text[i] = text[i];
                        inbox.msgs[MAX_INBOX - 1].text[tlen] = '\0';
                    }
                    if (cur_msg >= inbox.count) cur_msg = inbox.count - 1;
                    api->storageWrite("mesh_inbox", &inbox, sizeof(inbox));
                    needsRedraw = 1;
                }
            }
        }

        /* Long press */
        if (api->buttonPressed()) {
            if (pressStart == 0) pressStart = now;
            if (!long_fired && (now - pressStart) >= LONG_MS) {
                long_fired = 1;
                if (view == VIEW_INBOX) {
                    api->loraSleep();
                    return;
                } else {
                    uint8_t pkt[200];
                    int pktLen = meshBuildPacket((const uint8_t(*)[16])rk, nodeId, now,
                                                QUICK_MSGS[cur_send], pkt, (int)sizeof(pkt));
                    drawSending(api, QUICK_MSGS[cur_send]);
                    if (pktLen > 0) api->loraSend(pkt, pktLen);
                    needsRedraw = 1;
                }
            }
        } else {
            pressStart = 0;
            long_fired = 0;
        }

        /* Button events */
        {
            uint8_t evt = api->pollEvent();
            if (view == VIEW_INBOX) {
                if (evt == PALA_CLICK) {
                    if (inbox.count > 1) {
                        cur_msg = (cur_msg + 1) % inbox.count;
                        needsRedraw = 1;
                    }
                } else if (evt == PALA_DOUBLE) {
                    view = VIEW_SEND;
                    needsRedraw = 1;
                }
            } else {
                if (evt == PALA_CLICK) {
                    cur_send = (cur_send + 1) % NUM_QUICK;
                    needsRedraw = 1;
                } else if (evt == PALA_DOUBLE) {
                    view = VIEW_INBOX;
                    needsRedraw = 1;
                }
            }
        }

        if (needsRedraw) {
            if (view == VIEW_INBOX)
                drawInbox(api, &inbox, cur_msg);
            else
                drawSend(api, cur_send);
            needsRedraw = 0;
        }

        api->delayMs(10);
    }
}
