#include <klib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

// ========== SM4 硬件指令 ==========
static inline uint32_t sm4ed(uint32_t rs1, uint32_t rs2, int bs) {
    uint32_t rd;
    switch(bs) {
        case 0:
            asm volatile(".insn r 0x33, 0x0, 0x18, %0, %1, %2" : "=r"(rd) : "r"(rs1), "r"(rs2));
            break;
        case 1:
            asm volatile(".insn r 0x33, 0x0, 0x38, %0, %1, %2" : "=r"(rd) : "r"(rs1), "r"(rs2));
            break;
        case 2:
            asm volatile(".insn r 0x33, 0x0, 0x58, %0, %1, %2" : "=r"(rd) : "r"(rs1), "r"(rs2));
            break;
        case 3:
            asm volatile(".insn r 0x33, 0x0, 0x78, %0, %1, %2" : "=r"(rd) : "r"(rs1), "r"(rs2));
            break;
    }
    return rd;
}

static inline uint32_t sm4ks(uint32_t rs1, uint32_t rs2, int bs) {
    uint32_t rd;
    switch(bs) {
        case 0:
            asm volatile(".insn r 0x33, 0x0, 0x1A, %0, %1, %2" : "=r"(rd) : "r"(rs1), "r"(rs2));
            break;
        case 1:
            asm volatile(".insn r 0x33, 0x0, 0x3A, %0, %1, %2" : "=r"(rd) : "r"(rs1), "r"(rs2));
            break;
        case 2:
            asm volatile(".insn r 0x33, 0x0, 0x5A, %0, %1, %2" : "=r"(rd) : "r"(rs1), "r"(rs2));
            break;
        case 3:
            asm volatile(".insn r 0x33, 0x0, 0x7A, %0, %1, %2" : "=r"(rd) : "r"(rs1), "r"(rs2));
            break;
    }
    return rd;
}

// ========== SM4 软件实现 ==========

static const uint8_t SM4_SBOX[256] = {
	0xD6, 0x90, 0xE9, 0xFE, 0xCC, 0xE1, 0x3D, 0xB7, 0x16, 0xB6, 0x14, 0xC2,
	0x28, 0xFB, 0x2C, 0x05, 0x2B, 0x67, 0x9A, 0x76, 0x2A, 0xBE, 0x04, 0xC3,
	0xAA, 0x44, 0x13, 0x26, 0x49, 0x86, 0x06, 0x99, 0x9C, 0x42, 0x50, 0xF4,
	0x91, 0xEF, 0x98, 0x7A, 0x33, 0x54, 0x0B, 0x43, 0xED, 0xCF, 0xAC, 0x62,
	0xE4, 0xB3, 0x1C, 0xA9, 0xC9, 0x08, 0xE8, 0x95, 0x80, 0xDF, 0x94, 0xFA,
	0x75, 0x8F, 0x3F, 0xA6, 0x47, 0x07, 0xA7, 0xFC, 0xF3, 0x73, 0x17, 0xBA,
	0x83, 0x59, 0x3C, 0x19, 0xE6, 0x85, 0x4F, 0xA8, 0x68, 0x6B, 0x81, 0xB2,
	0x71, 0x64, 0xDA, 0x8B, 0xF8, 0xEB, 0x0F, 0x4B, 0x70, 0x56, 0x9D, 0x35,
	0x1E, 0x24, 0x0E, 0x5E, 0x63, 0x58, 0xD1, 0xA2, 0x25, 0x22, 0x7C, 0x3B,
	0x01, 0x21, 0x78, 0x87, 0xD4, 0x00, 0x46, 0x57, 0x9F, 0xD3, 0x27, 0x52,
	0x4C, 0x36, 0x02, 0xE7, 0xA0, 0xC4, 0xC8, 0x9E, 0xEA, 0xBF, 0x8A, 0xD2,
	0x40, 0xC7, 0x38, 0xB5, 0xA3, 0xF7, 0xF2, 0xCE, 0xF9, 0x61, 0x15, 0xA1,
	0xE0, 0xAE, 0x5D, 0xA4, 0x9B, 0x34, 0x1A, 0x55, 0xAD, 0x93, 0x32, 0x30,
	0xF5, 0x8C, 0xB1, 0xE3, 0x1D, 0xF6, 0xE2, 0x2E, 0x82, 0x66, 0xCA, 0x60,
	0xC0, 0x29, 0x23, 0xAB, 0x0D, 0x53, 0x4E, 0x6F, 0xD5, 0xDB, 0x37, 0x45,
	0xDE, 0xFD, 0x8E, 0x2F, 0x03, 0xFF, 0x6A, 0x72, 0x6D, 0x6C, 0x5B, 0x51,
	0x8D, 0x1B, 0xAF, 0x92, 0xBB, 0xDD, 0xBC, 0x7F, 0x11, 0xD9, 0x5C, 0x41,
	0x1F, 0x10, 0x5A, 0xD8, 0x0A, 0xC1, 0x31, 0x88, 0xA5, 0xCD, 0x7B, 0xBD,
	0x2D, 0x74, 0xD0, 0x12, 0xB8, 0xE5, 0xB4, 0xB0, 0x89, 0x69, 0x97, 0x4A,
	0x0C, 0x96, 0x77, 0x7E, 0x65, 0xB9, 0xF1, 0x09, 0xC5, 0x6E, 0xC6, 0x84,
	0x18, 0xF0, 0x7D, 0xEC, 0x3A, 0xDC, 0x4D, 0x20, 0x79, 0xEE, 0x5F, 0x3E,
	0xD7, 0xCB, 0x39, 0x48
};

static const uint32_t SM4_FK[4] = {
    0xA3B1BAC6, 0x56AA3350, 0x677D9197, 0xB27022DC
};

static const uint32_t SM4_CK[32] = {
    0x00070E15, 0x1C232A31, 0x383F464D, 0x545B6269,
    0x70777E85, 0x8C939AA1, 0xA8AFB6BD, 0xC4CBD2D9,
    0xE0E7EEF5, 0xFC030A11, 0x181F262D, 0x343B4249,
    0x50575E65, 0x6C737A81, 0x888F969D, 0xA4ABB2B9,
    0xC0C7CED5, 0xDCE3EAF1, 0xF8FF060D, 0x141B2229,
    0x30373E45, 0x4C535A61, 0x686F767D, 0x848B9299,
    0xA0A7AEB5, 0xBCC3CAD1, 0xD8DFE6ED, 0xF4FB0209,
    0x10171E25, 0x2C333A41, 0x484F565D, 0x646B7279
};

// 字节序转换
static inline uint32_t GET32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | 
           ((uint32_t)p[2] << 8) | ((uint32_t)p[3]);
}

static inline void PUT32(uint8_t *p, uint32_t v) {
    p[0] = (v >> 24) & 0xff;
    p[1] = (v >> 16) & 0xff;
    p[2] = (v >> 8) & 0xff;
    p[3] = v & 0xff;
}

// 循环左移
#define ROTL(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

// S盒变换
static uint32_t SM4_SBOX_T(uint32_t x) {
    uint8_t a[4];
    a[0] = SM4_SBOX[(x >> 24) & 0xff];
    a[1] = SM4_SBOX[(x >> 16) & 0xff];
    a[2] = SM4_SBOX[(x >> 8) & 0xff];
    a[3] = SM4_SBOX[x & 0xff];
    return GET32(a);
}

// 线性变换L
static uint32_t SM4_L(uint32_t x) {
    return x ^ ROTL(x, 2) ^ ROTL(x, 10) ^ ROTL(x, 18) ^ ROTL(x, 24);
}

// 线性变换L'（密钥扩展）
static uint32_t SM4_L_KEY(uint32_t x) {
    return x ^ ROTL(x, 13) ^ ROTL(x, 23);
}

// T变换
static uint32_t SM4_T(uint32_t x) {
    return SM4_L(SM4_SBOX_T(x));
}

// T'变换（密钥扩展）
static uint32_t SM4_T_KEY(uint32_t x) {
    return SM4_L_KEY(SM4_SBOX_T(x));
}

// 轮函数
static uint32_t SM4_F(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3, uint32_t rk) {
    return x0 ^ SM4_T(x1 ^ x2 ^ x3 ^ rk);
}

// 密钥扩展
void sm4_key_schedule(const uint8_t key[16], uint32_t rk[32]) {
    uint32_t K[36];
    
    K[0] = GET32(key) ^ SM4_FK[0];
    K[1] = GET32(key + 4) ^ SM4_FK[1];
    K[2] = GET32(key + 8) ^ SM4_FK[2];
    K[3] = GET32(key + 12) ^ SM4_FK[3];
    
    for (int i = 0; i < 32; i++) {
        K[i+4] = K[i] ^ SM4_T_KEY(K[i+1] ^ K[i+2] ^ K[i+3] ^ SM4_CK[i]);
        rk[i] = K[i+4];
    }
}

// 加密/解密核心函数
void sm4_crypt(const uint8_t in[16], uint8_t out[16], const uint32_t rk[32]) {
    uint32_t X[36];
    
    X[0] = GET32(in);
    X[1] = GET32(in + 4);
    X[2] = GET32(in + 8);
    X[3] = GET32(in + 12);
    
    for (int i = 0; i < 32; i++) {
        X[i+4] = SM4_F(X[i], X[i+1], X[i+2], X[i+3], rk[i]);
    }
    
    PUT32(out, X[35]);
    PUT32(out + 4, X[34]);
    PUT32(out + 8, X[33]);
    PUT32(out + 12, X[32]);
}

// ========== 测试硬件加速版本 ==========
void test_sm4_hw_accelerated() {
    printf("\n=== Testing SM4 with Hardware Acceleration ===\n");
    
    // 分析硬件指令的行为
    printf("Analyzing hardware instruction behavior:\n");
    
    // 测试一个简单的例子
    uint32_t test_input = 0x12345678;
    uint32_t test_key = 0x9ABCDEF0;
    
    // 软件计算S盒+L变换
    uint32_t sw_sbox = SM4_SBOX_T(test_key);
    uint32_t sw_L = SM4_L(sw_sbox);
    uint32_t sw_result = test_input ^ sw_L;
    
    printf("Software calculation:\n");
    printf("  Input: 0x%08x, Key: 0x%08x\n", test_input, test_key);
    printf("  S-box result: 0x%08x\n", sw_sbox);
    printf("  L transform: 0x%08x\n", sw_L);
    printf("  Final (input ^ L): 0x%08x\n", sw_result);
    
    // 测试硬件指令
    printf("\nHardware SM4ED results:\n");
    for (int bs = 0; bs < 4; bs++) {
        uint32_t hw = sm4ed(test_input, test_key, bs);
        printf("  BS=%d: 0x%08x\n", bs, hw);
    }
    
    // 根据硬件指令的行为，看看是否能组合出正确的结果
    // 可能需要4个SM4ED指令的组合
    uint32_t combined = 0;
    for (int bs = 0; bs < 4; bs++) {
        combined ^= sm4ed(0, test_key, bs);
    }
    printf("\nCombined (XOR of all BS): 0x%08x\n", combined);
}

// ========== 标准测试向量 ==========
void test_sm4_vectors() {
    printf("\n=== SM4 Standard Test Vectors ===\n");
    
    // 测试向量（来自SM4标准文档）
    uint8_t key[16] = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
        0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10
    };
    
    uint8_t plaintext[16] = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
        0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10
    };
    
    // 标准答案
    uint8_t expected_cipher[16] = {
        0x68, 0x1E, 0xDF, 0x34, 0xD2, 0x06, 0x96, 0x5E,
        0x86, 0xB3, 0xE9, 0x4F, 0x53, 0x6E, 0x42, 0x46
    };
    
    uint32_t rk[32];
    uint8_t ciphertext[16];
    uint8_t decrypted[16];
    
    // 生成轮密钥
    sm4_key_schedule(key, rk);
    
    // 显示前4个轮密钥（用于验证）
    printf("Round keys (first 4):\n");
    for (int i = 0; i < 4; i++) {
        printf("  rk[%d] = 0x%08X\n", i, rk[i]);
    }
    
    // 加密
    sm4_crypt(plaintext, ciphertext, rk);
    
    printf("\nEncryption:\n");
    printf("  Plaintext:  ");
    for (int i = 0; i < 16; i++) printf("%02x", plaintext[i]);
    printf("\n  Ciphertext: ");
    for (int i = 0; i < 16; i++) printf("%02x", ciphertext[i]);
    printf("\n  Expected:   ");
    for (int i = 0; i < 16; i++) printf("%02x", expected_cipher[i]);
    
    int match = 1;
    for (int i = 0; i < 16; i++) {
        if (ciphertext[i] != expected_cipher[i]) {
            match = 0;
            break;
        }
    }
    printf("\n  Result: %s\n", match ? "PASS ✓" : "FAIL ✗");
    
    // 解密（反向使用轮密钥）
    uint32_t rk_dec[32];
    for (int i = 0; i < 32; i++) {
        rk_dec[i] = rk[31 - i];
    }
    sm4_crypt(ciphertext, decrypted, rk_dec);
    
    printf("\nDecryption:\n");
    printf("  Decrypted:  ");
    for (int i = 0; i < 16; i++) printf("%02x", decrypted[i]);
    
    match = 1;
    for (int i = 0; i < 16; i++) {
        if (decrypted[i] != plaintext[i]) {
            match = 0;
            break;
        }
    }
    printf("\n  Result: %s\n", match ? "PASS ✓" : "FAIL ✗");
}

int main() {
    printf("SM4 Cipher Test\n");
    printf("===============\n");
    
    // 首先测试软件版本的正确性
    test_sm4_vectors();
    
    // 然后分析硬件指令的行为
    test_sm4_hw_accelerated();
    
    return 0;
}
