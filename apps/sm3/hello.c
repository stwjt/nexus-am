#include <klib.h>

#include <klib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

// SM3上下文结构
typedef struct {
    uint32_t state[8];
    uint64_t count;
    uint8_t buffer[64];
} SM3_CTX;

// SM3初始化向量
static const uint32_t SM3_IV[8] = {
    0x7380166f, 0x4914b2b9, 0x172442d7, 0xda8a0600,
    0xa96f30bc, 0x163138aa, 0xe38dee4d, 0xb0fb0e4e
};

// 软件实现的P0和P1函数（用于验证）
static inline uint32_t P0_soft(uint32_t X) {
    return X ^ ((X << 9) | (X >> 23)) ^ ((X << 17) | (X >> 15));
}

static inline uint32_t P1_soft(uint32_t X) {
    return X ^ ((X << 15) | (X >> 17)) ^ ((X << 23) | (X >> 9));
}

// 香山SM3P0指令 - 根据香山的ScalarCryptoDecode
// ========== SM3 硬件加速指令 ==========
// SM3P0: rd = rs1 ^ (rs1 <<< 9) ^ (rs1 <<< 17)
static inline uint32_t sm3p0(uint32_t rs1) {
    uint32_t rd;
    // SM3P0是I-type指令: 000100001000 rs1 001 rd 0010011
    asm volatile(
        ".insn i 0x13, 0x1, %0, %1, 0x108"  // opcode=0x13, func3=0x1, imm=0x108
        : "=r"(rd)
        : "r"(rs1)
    );
    return rd;
}
// SM3P1: rd = rs1 ^ (rs1 <<< 15) ^ (rs1 <<< 23)
static inline uint32_t sm3p1(uint32_t rs1) {
    uint32_t rd;
    // SM3P1是I-type指令: 000100001001 rs1 001 rd 0010011
    asm volatile(
        ".insn i 0x13, 0x1, %0, %1, 0x109"  // opcode=0x13, func3=0x1, imm=0x109
        : "=r"(rd)
        : "r"(rs1)
    );
    return rd;
}

// 尝试另一种编码方式（根据RISC-V Crypto扩展规范）
// SM3P0: rd = rs1 ^ (rs1 <<< 9) ^ (rs1 <<< 17)
static inline uint32_t sm3p0_v2(uint32_t x) {
    uint32_t result;
    // 根据规范，SM3P0的编码可能是不同的
    // 尝试使用直接的机器码
    asm volatile(
        ".word 0x10801013"  // 可能的SM3P0编码
        : "=r"(result)
        : "r"(x)
    );
    return result;
}

static inline uint32_t sm3p1_v2(uint32_t x) {
    uint32_t result;
    asm volatile(
        ".word 0x10901013"  // 可能的SM3P1编码
        : "=r"(result)
        : "r"(x)
    );
    return result;
}

// 由于硬件实现可能有问题，暂时使用软件实现
// #define USE_SOFTWARE_P0P1 1

#if USE_SOFTWARE_P0P1
    #define sm3p0_impl P0_soft
    #define sm3p1_impl P1_soft
#else
    #define sm3p0_impl sm3p0
    #define sm3p1_impl sm3p1
#endif

// SM3消息扩展
__attribute__((noinline))
void sm3_expand(uint32_t W[68], const uint8_t block[64]) {
    // 前16个字直接从消息块加载（大端序）
    for (int i = 0; i < 16; i++) {
        W[i] = ((uint32_t)block[i * 4] << 24) |
               ((uint32_t)block[i * 4 + 1] << 16) |
               ((uint32_t)block[i * 4 + 2] << 8) |
               ((uint32_t)block[i * 4 + 3]);
    }
    
    // 消息扩展 W[16]到W[67]
    for (int i = 16; i < 68; i++) {
        uint32_t tmp = W[i - 16] ^ W[i - 9] ^ ((W[i - 3] << 15) | (W[i - 3] >> 17));
        W[i] = sm3p1_impl(tmp) ^ ((W[i - 13] << 7) | (W[i - 13] >> 25)) ^ W[i - 6];
    }
}

// SM3压缩函数
__attribute__((noinline))
void sm3_compress(uint32_t state[8], const uint8_t block[64]) {
    uint32_t W[68];
    uint32_t W1[64];
    uint32_t A, B, C, D, E, F, G, H;
    
    // 消息扩展
    sm3_expand(W, block);
    
    // 计算W'
    for (int i = 0; i < 64; i++) {
        W1[i] = W[i] ^ W[i + 4];
    }
    
    // 初始化工作变量
    A = state[0]; B = state[1]; C = state[2]; D = state[3];
    E = state[4]; F = state[5]; G = state[6]; H = state[7];
    
    // 64轮压缩
    for (int i = 0; i < 64; i++) {
        uint32_t T = (i < 16) ? 0x79cc4519 : 0x7a879d8a;
        uint32_t SS1, SS2, TT1, TT2;
        
        // 计算SS1
        uint32_t rot_val = (i < 16) ? i : (i % 32);
        uint32_t T_rot = (T << rot_val) | (T >> (32 - rot_val));
        uint32_t A_rot12 = (A << 12) | (A >> 20);
        SS1 = A_rot12 + E + T_rot;
        SS1 = (SS1 << 7) | (SS1 >> 25);
        SS2 = SS1 ^ A_rot12;
        
        // 布尔函数和消息扩展
        if (i < 16) {
            TT1 = (A ^ B ^ C) + D + SS2 + W1[i];
            TT2 = (E ^ F ^ G) + H + SS1 + W[i];
        } else {
            TT1 = ((A & B) | (A & C) | (B & C)) + D + SS2 + W1[i];
            TT2 = ((E & F) | (~E & G)) + H + SS1 + W[i];
        }
        
        // 更新寄存器
        D = C;
        C = (B << 9) | (B >> 23);
        B = A;
        A = TT1;
        H = G;
        G = (F << 19) | (F >> 13);
        F = E;
        E = sm3p0_impl(TT2);
    }
    
    // 更新状态
    state[0] ^= A; state[1] ^= B; state[2] ^= C; state[3] ^= D;
    state[4] ^= E; state[5] ^= F; state[6] ^= G; state[7] ^= H;
}

// SM3初始化
void sm3_init(SM3_CTX *ctx) {
    memcpy(ctx->state, SM3_IV, 32);
    ctx->count = 0;
    memset(ctx->buffer, 0, 64);
}

// SM3更新
void sm3_update(SM3_CTX *ctx, const uint8_t *data, size_t len) {
    size_t buffer_len = ctx->count % 64;
    ctx->count += len;
    
    if (buffer_len) {
        size_t fill = 64 - buffer_len;
        if (len < fill) {
            memcpy(ctx->buffer + buffer_len, data, len);
            return;
        }
        memcpy(ctx->buffer + buffer_len, data, fill);
        sm3_compress(ctx->state, ctx->buffer);
        data += fill;
        len -= fill;
    }
    
    while (len >= 64) {
        sm3_compress(ctx->state, data);
        data += 64;
        len -= 64;
    }
    
    if (len) {
        memcpy(ctx->buffer, data, len);
    }
}

// SM3最终化
void sm3_final(SM3_CTX *ctx, uint8_t digest[32]) {
    uint64_t bit_count = ctx->count * 8;
    size_t buffer_len = ctx->count % 64;
    
    ctx->buffer[buffer_len++] = 0x80;
    if (buffer_len > 56) {
        memset(ctx->buffer + buffer_len, 0, 64 - buffer_len);
        sm3_compress(ctx->state, ctx->buffer);
        buffer_len = 0;
    }
    memset(ctx->buffer + buffer_len, 0, 56 - buffer_len);
    
    for (int i = 0; i < 8; i++) {
        ctx->buffer[56 + i] = (bit_count >> (8 * (7 - i))) & 0xff;
    }
    sm3_compress(ctx->state, ctx->buffer);
    
    for (int i = 0; i < 8; i++) {
        digest[i * 4] = (ctx->state[i] >> 24) & 0xff;
        digest[i * 4 + 1] = (ctx->state[i] >> 16) & 0xff;
        digest[i * 4 + 2] = (ctx->state[i] >> 8) & 0xff;
        digest[i * 4 + 3] = ctx->state[i] & 0xff;
    }
}

int main() {
    SM3_CTX ctx;
    uint8_t digest[32];
    
    // 测试硬件指令
    printf("Testing SM3 instructions:\n");
    uint32_t test_val = 0x12345678;
    uint32_t sw_p0 = P0_soft(test_val);
    uint32_t sw_p1 = P1_soft(test_val);
    
    printf("Software P0(0x%08x) = 0x%08x\n", test_val, sw_p0);
    printf("Software P1(0x%08x) = 0x%08x\n", test_val, sw_p1);
    
#if !USE_SOFTWARE_P0P1
    uint32_t hw_p0 = sm3p0(test_val);
    uint32_t hw_p1 = sm3p1(test_val);
    printf("Hardware P0(0x%08x) = 0x%08x %s\n", test_val, hw_p0, (hw_p0 == sw_p0) ? "✓" : "✗");
    printf("Hardware P1(0x%08x) = 0x%08x %s\n", test_val, hw_p1, (hw_p1 == sw_p1) ? "✓" : "✗");
#else
    printf("Using software implementation for P0/P1\n");
#endif
    
    printf("\n");
    
    // SM3哈希测试
    const char *msg = "abc";
    sm3_init(&ctx);
    sm3_update(&ctx, (uint8_t *)msg, strlen(msg));
    sm3_final(&ctx, digest);
    
    printf("SM3 Hash Test:\n");
    printf("Message: \"%s\"\n", msg);
    printf("Digest:  ");
    for (int i = 0; i < 32; i++) {
        printf("%02x", digest[i]);
    }
    printf("\n");
    printf("Expected: 66c7f0f462eeedd9d1f2d46bdc10e4e24167c4875cf2f7a2297da02b8f4ba8e0\n");
    
    // 验证结果
    const uint8_t expected[32] = {
        0x66, 0xc7, 0xf0, 0xf4, 0x62, 0xee, 0xed, 0xd9,
        0xd1, 0xf2, 0xd4, 0x6b, 0xdc, 0x10, 0xe4, 0xe2,
        0x41, 0x67, 0xc4, 0x87, 0x5c, 0xf2, 0xf7, 0xa2,
        0x29, 0x7d, 0xa0, 0x2b, 0x8f, 0x4b, 0xa8, 0xe0
    };
    
    int match = 1;
    for (int i = 0; i < 32; i++) {
        if (digest[i] != expected[i]) {
            match = 0;
            break;
        }
    }
    
    printf("Result: %s\n\n", match ? "PASS ✓" : "FAIL ✗");
    
    return 0;
}
