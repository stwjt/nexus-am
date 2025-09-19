// #include "xil_types.h"
// #include "xil_io.h"
// #include "xparameters.h"
// s#include "xstatus.h"
#include <stdint.h>
#include "stdlib.h"
#include <klib.h>
#include "data1.h"
typedef uint32_t u32;
typedef uintptr_t UINTPTR;
//DDR
#define DDR_BASE_ADDRESS    0x80000000
//GPGPU address
#define GPU_BASEADDR                0x31000000L
#define GPU_HIGHADDR		0x3fffffff
/*************PARAM OFFSETS**********************/
#define GPU_VALID_OFFSET                     0x00        // host -> gpu
#define GPU_WG_ID_OFFSET                        0x04
#define GPU_NUM_WF_OFFSET               0x08
#define GPU_WF_SIZE_OFFSET              0x0c
#define GPU_START_PC_OFFSET                    0x10
#define GPU_VGPR_SIZE_T_OFFSET          0x14
#define GPU_SGPR_SIZE_T_OFFSET          0x18
#define GPU_LDS_SIZE_OFFSET                    0x1c
#define GPU_VGPR_SIZE_WF_OFFSET         0x20
#define GPU_SGPR_SIZE_WF_OFFSET         0x24
#define GPU_DATA_BASEADDR_OFFSET        0x28        // dcache
#define GPU_PC_BASEADDR_OFFSET          0x2c        // icache
#define HOST_REQ_CSR_KNL                0x30
#define HOST_REQ_KERNEL_SIZE_3d         0x34
#define GPU_WG_ID_DONE_OFFSET           0x38
#define GPU_WG_VALID_OFFSET             0x3c
/*************CONSTANTS**************************/
#define GPU_SEND_TIMEOUT                8
#define XST_SUCCESS 0L
#define XST_FAILURE 1L

// // All data here
// static u32 ProgramInstr[1024] = {
//     0
// };
// static u32 ProgramData[32768] = {
//     0
// };
static inline uint32_t Xil_In32(uintptr_t Addr)
{

    return *(volatile uint32_t *)Addr;
}


static inline void Xil_Out32(uintptr_t  Addr, uintptr_t Data)
{
    // printf("Xil_Out32 addr %lx data %lx\n",Addr,(uintptr_t)Data);
    *(volatile uint32_t *)Addr = (uintptr_t)Data;
}

#define Gpu_ReadReg(BaseAddress, RegOffset)             \
        Xil_In32((BaseAddress) + (u32)(RegOffset))
#define Gpu_WriteReg(BaseAddress, RegOffset, Data)      \
        Xil_Out32((uintptr_t)(BaseAddress) + (uintptr_t)(RegOffset), (uint32_t)(uintptr_t)(Data))

#define GPU_ADDR_WIDTH              32

typedef struct{
    u32* Instr;
    u32* Data;
    int ISize;              // word
    int DSize;              // word
}TaskMemory;

typedef struct{
    UINTPTR BaseAddr;       // GPU Base Address
    int Initialized;
    int AddrWidth;
}Gpu;

typedef struct{
    u32 WgId;
    u32 NumWf;
    u32 WfSize;
    u32 StartPC;
    u32 VgprSizeTotal;
    u32 SgprSizeTotal;
    u32 LdsSize;
    u32 VgprSizePerWf;
    u32 SgprSizePerWf;
    u32 metaDataBaseAddr;
    u32 Host_req_pds_baseaddr;
    TaskMemory Mem;
}TaskConfig;


u32 GpuInit(Gpu* GpuInstance, UINTPTR BaseAddr){
    GpuInstance->AddrWidth = 32;
    GpuInstance->BaseAddr = BaseAddr;
    GpuInstance->Initialized = 1;
    return XST_SUCCESS;
}
// init and load memory for task
// ISize DSize = IMem DMem size (in words)
// u32 GpuTaskMemoryInit(TaskMemory* Mem, int ISize, int DSize){
//     u32* Instr = (u32*)malloc(ISize * sizeof(u32));
//     u32* Data = (u32*)malloc(DSize * sizeof(u32));
//     if(Instr == NULL || Data == NULL){
//         printf("Failed to allocate memory.\r\n");
//         return XST_FAILURE;
//     }
//     printf("Instr: %08x[%d], Data: %08x[%d].\r\n", Instr, ISize, Data, DSize);
//     memcpy(Instr, ProgramInstr, ISize * sizeof(u32));
//     memcpy(Data, ProgramData, DSize * sizeof(u32));
//     Mem->Instr = Instr;
//     Mem->Data = Data;
//     Mem->ISize = ISize;
//     Mem->DSize = DSize;
//     return XST_SUCCESS;
// }

u32 GpuTaskMemoryInit(TaskMemory* Mem, int ISize, int DSize){
    u32* Instr = (u32*)malloc(ISize * sizeof(u32));
    u32* Data = (u32*)malloc(DSize * sizeof(u32));
    if(Instr == NULL || Data == NULL){
        printf("Failed to allocate memory.\r\n");
        return XST_FAILURE;
    }
    printf("Instr: %08x[%d], Data: %08x[%d].\r\n", Instr, ISize, Data, DSize);
    Mem->Instr = Instr;
    Mem->Data = Data;
    Mem->ISize = ISize;
    Mem->DSize = DSize;
    return XST_SUCCESS;
}
static inline uint64_t rdcycle(void) {
    uint64_t c;
    asm volatile("rdcycle %0" : "=r"(c));
    return c;
}

void delay_cycles(uint64_t cycles) {
    uint64_t start = rdcycle();
    while ((rdcycle() - start) < cycles)
        ;
}

void delay_us(uint32_t us) {
    const uint64_t cpu_hz = 16000000ULL;  // 16 MHz
    delay_cycles((uint64_t)us * cpu_hz / 1000000ULL);
}

u32 GpuSendTask(Gpu* GpuInstance, TaskConfig* TaskCfg){
    int wait;

    // Gpu_WriteReg(0x30000000, 4, 52);
    Gpu_WriteReg(GpuInstance->BaseAddr, GPU_WG_ID_OFFSET, TaskCfg->WgId);

    Gpu_WriteReg(GpuInstance->BaseAddr, GPU_NUM_WF_OFFSET, TaskCfg->NumWf);

    Gpu_WriteReg(GpuInstance->BaseAddr, GPU_WF_SIZE_OFFSET, TaskCfg->WfSize);

    Gpu_WriteReg(GpuInstance->BaseAddr, GPU_START_PC_OFFSET, TaskCfg->StartPC);

    Gpu_WriteReg(GpuInstance->BaseAddr, GPU_VGPR_SIZE_T_OFFSET, TaskCfg->VgprSizeTotal);

    Gpu_WriteReg(GpuInstance->BaseAddr, GPU_SGPR_SIZE_T_OFFSET, TaskCfg->SgprSizeTotal);

    Gpu_WriteReg(GpuInstance->BaseAddr, GPU_LDS_SIZE_OFFSET, TaskCfg->LdsSize);

    Gpu_WriteReg(GpuInstance->BaseAddr, GPU_VGPR_SIZE_WF_OFFSET, TaskCfg->VgprSizePerWf);

    Gpu_WriteReg(GpuInstance->BaseAddr, GPU_SGPR_SIZE_WF_OFFSET, TaskCfg->SgprSizePerWf);

    Gpu_WriteReg(GpuInstance->BaseAddr, GPU_DATA_BASEADDR_OFFSET, (u32)0x90000000);
    Gpu_WriteReg(GpuInstance->BaseAddr, GPU_PC_BASEADDR_OFFSET,(u32)0x90001000);
    Gpu_WriteReg(GpuInstance->BaseAddr, HOST_REQ_CSR_KNL, TaskCfg->metaDataBaseAddr);
    Gpu_WriteReg(GpuInstance->BaseAddr, HOST_REQ_KERNEL_SIZE_3d, (u32)0);
    Gpu_WriteReg(GpuInstance->BaseAddr, GPU_WG_ID_DONE_OFFSET, (u32)0);
    Gpu_WriteReg(GpuInstance->BaseAddr, GPU_WG_VALID_OFFSET, (u32)0);
    Gpu_WriteReg(GpuInstance->BaseAddr, GPU_VALID_OFFSET, (u32)1);


    Gpu_WriteReg(GpuInstance->BaseAddr, GPU_VALID_OFFSET, (u32)1);

    wait = GPU_SEND_TIMEOUT;
    while(wait){
        if(Gpu_ReadReg(GpuInstance->BaseAddr, GPU_VALID_OFFSET) == 0){
            // delay_us(1);
            break;
        }
        
        wait--;
    }

    if(wait == 0){
        printf("Sending WG#%d failed.\r\n", TaskCfg->WgId);
        return XST_FAILURE;
    }
    u32 val;
    val = Gpu_ReadReg(GpuInstance->BaseAddr, GPU_VALID_OFFSET);

    return XST_SUCCESS;
}
void GpuDebugPrintRegs(Gpu* GpuInstance)
{
    u32 val;

    val = Gpu_ReadReg(GpuInstance->BaseAddr, GPU_WG_ID_OFFSET);
    printf("GPU_WG_ID            = 0x%08x\n", val);

    val = Gpu_ReadReg(GpuInstance->BaseAddr, GPU_NUM_WF_OFFSET);
    printf("GPU_NUM_WF           = 0x%08x\n", val);

    val = Gpu_ReadReg(GpuInstance->BaseAddr, GPU_WF_SIZE_OFFSET);
    printf("GPU_WF_SIZE          = 0x%08x\n", val);

    val = Gpu_ReadReg(GpuInstance->BaseAddr, GPU_START_PC_OFFSET);
    printf("GPU_START_PC         = 0x%08x\n", val);

    val = Gpu_ReadReg(GpuInstance->BaseAddr, GPU_VGPR_SIZE_T_OFFSET);
    printf("GPU_VGPR_SIZE_TOTAL  = 0x%08x\n", val);

    val = Gpu_ReadReg(GpuInstance->BaseAddr, GPU_SGPR_SIZE_T_OFFSET);
    printf("GPU_SGPR_SIZE_TOTAL  = 0x%08x\n", val);

    val = Gpu_ReadReg(GpuInstance->BaseAddr, GPU_LDS_SIZE_OFFSET);
    printf("GPU_LDS_SIZE         = 0x%08x\n", val);

    val = Gpu_ReadReg(GpuInstance->BaseAddr, GPU_VGPR_SIZE_WF_OFFSET);
    printf("GPU_VGPR_SIZE_PER_WF = 0x%08x\n", val);

    val = Gpu_ReadReg(GpuInstance->BaseAddr, GPU_SGPR_SIZE_WF_OFFSET);
    printf("GPU_SGPR_SIZE_PER_WF = 0x%08x\n", val);

    val = Gpu_ReadReg(GpuInstance->BaseAddr, GPU_PC_BASEADDR_OFFSET);
    printf("GPU_PC_BASEADDR      = 0x%08x\n", val);

    val = Gpu_ReadReg(GpuInstance->BaseAddr, GPU_DATA_BASEADDR_OFFSET);
    printf("GPU_DATA_BASEADDR    = 0x%08x\n", val);

    val = Gpu_ReadReg(GpuInstance->BaseAddr, HOST_REQ_CSR_KNL);
    printf("HOST_REQ_CSR_KNL     = 0x%08x\n", val);

    val = Gpu_ReadReg(GpuInstance->BaseAddr, HOST_REQ_KERNEL_SIZE_3d);
    printf("HOST_REQ_KERNEL_SIZE_3d = 0x%08x\n", val);

    val = Gpu_ReadReg(GpuInstance->BaseAddr, GPU_WG_ID_DONE_OFFSET);
    printf("GPU_WG_ID_DONE       = 0x%08x\n", val);

    val = Gpu_ReadReg(GpuInstance->BaseAddr, GPU_WG_VALID_OFFSET);
    printf("GPU_WG_VALID         = 0x%08x\n", val);

    val = Gpu_ReadReg(GpuInstance->BaseAddr, GPU_VALID_OFFSET);
    printf("GPU_VALID            = 0x%08x\n", val);
}

u32 GpuWatchTask(Gpu* GpuInstance, u32* WgIdReturn, int wait){
    while(wait){
        if(Gpu_ReadReg(GpuInstance->BaseAddr, GPU_WG_VALID_OFFSET) == 1)
            break;
        wait--;
    }
    if(wait == 0){
        printf("Waiting time limit exceeded.\r\n");
        return XST_FAILURE;
    }
    *WgIdReturn = Gpu_ReadReg(GpuInstance->BaseAddr, GPU_WG_ID_DONE_OFFSET);
    printf("Get response from WG#%d.\r\n", *WgIdReturn);
    Gpu_WriteReg(GpuInstance->BaseAddr, GPU_WG_VALID_OFFSET, (u32)0);
    return XST_SUCCESS;
}

// clear the memory
void GpuDeleteTask(TaskConfig* TaskCfg){
    TaskCfg->Mem.Instr = 0;
    TaskCfg->Mem.Data = 0;
    free(TaskCfg->Mem.Instr);
    free(TaskCfg->Mem.Data);
    return;
}







// 初始化内存
void init_mem(uint32_t* metadata, uint32_t* data, int metadata_size, int data_size) {
    
    uint64_t buf_num_soft = ((uint64_t)metadata[27] << 32) | metadata[26];
    
    uint64_t buf_ba_w[16];
    uint64_t buf_size[16];
    uint64_t buf_size_tmp[16];
    uint64_t burst_len[16];
    uint64_t burst_len_div[16];
    uint64_t burst_len_mod[16];
    uint64_t burst_times[16];
    
    // 解析buffer信息
    for (int i = 0; i < buf_num_soft; i++) {
        buf_ba_w[i] = ((uint64_t)metadata[i * 2 + 29] << 32) | metadata[i * 2 + 28];
        buf_size[i] = ((uint64_t)metadata[i * 2 + 29 + buf_num_soft * 2] << 32) | 
                      metadata[i * 2 + 28 + buf_num_soft * 2];
        buf_size_tmp[i] = (buf_size[i] % 4 == 0) ? buf_size[i] : (buf_size[i] / 4) * 4 + 4;
        burst_len[i] = buf_size_tmp[i] / 4;
        burst_len_div[i] = burst_len[i] / 16;
        burst_len_mod[i] = burst_len[i] % 16;
        burst_times[i] = (burst_len_mod[i] == 0) ? burst_len_div[i] : burst_len_div[i] + 1;
    }
    
    // 写入数据到内存
    int m = 0;
    for (int j = 0; j < buf_num_soft; j++) {
        uint64_t addr = buf_ba_w[j];
        
        for (int k = 0; k < burst_times[j]; k++) {
            uint64_t burst_data = (burst_len_mod[j] == 0) ? 16 : 
                                 ((k < burst_times[j] - 1) ? 16 : burst_len_mod[j]);
            
            for (int l = 0; l < burst_data; l++) {
                Xil_Out32(addr, data[m]);
                addr += 4;
                m++;
            }
            
            addr += 16 * 4 - burst_data * 4;
        }
    }
}


// void init_mem(uint32_t* metadata, uint32_t* data, int metadata_size, int data_size) {
//     uint64_t buf_num_soft = ((uint64_t)metadata[27] << 32) | metadata[26];
    
//     uint64_t buf_ba_w[16];
//     uint64_t buf_size[16];
//     uint64_t buf_size_tmp[16];
//     uint64_t burst_len[16];
//     uint64_t burst_len_div[16];
//     uint64_t burst_len_mod[16];
//     uint64_t burst_times[16];
    
//     // 解析buffer信息
//     for (int i = 0; i < buf_num_soft; i++) {
//         buf_ba_w[i] = ((uint64_t)metadata[i * 2 + 29] << 32) | metadata[i * 2 + 28];
//         buf_size[i] = ((uint64_t)metadata[i * 2 + 29 + buf_num_soft * 2] << 32) | 
//                       metadata[i * 2 + 28 + buf_num_soft * 2];
//         buf_size_tmp[i] = (buf_size[i] % 4 == 0) ? buf_size[i] : (buf_size[i] / 4) * 4 + 4;
//         burst_len[i] = buf_size_tmp[i] / 4;
//         burst_len_div[i] = burst_len[i] / 16;
//         burst_len_mod[i] = burst_len[i] % 16;
//         burst_times[i] = (burst_len_mod[i] == 0) ? burst_len_div[i] : burst_len_div[i] + 1;
//     }
    
//     // 写入数据到内存
//     int m = 0;
//     for (int j = 0; j < buf_num_soft; j++) {
//         uint64_t addr = buf_ba_w[j];
        
//         for (int k = 0; k < burst_times[j]; k++) {
//             uint64_t burst_data = (burst_len_mod[j] == 0) ? 16 : 
//                                  ((k < burst_times[j] - 1) ? 16 : burst_len_mod[j]);
            
//             for (int l = 0; l < burst_data; l++) {
//                 Xil_Out32(addr, data[m]);
//                 addr += 4;
//                 m++;
//             }
            
//             addr += 16 * 4 - burst_data * 4;
//         }
//     }
// }
// 设置高斯消元的数据
void setup_gaussian_data(int k) {
    int n = 1 << k;  // n = 2^k
    
    printf("\n=== Setting up Gaussian data for n=%d ===\n", n);
    
    // 根据gaussian8.data格式设置数据
    // 第一行：k值
    // 后续31行：0
    // 然后是n*n的矩阵A
    // 然后是n*n的零矩阵M
    // 最后是n个元素的向量B
}

//result
void process_data(uint32_t *mem_tmp_1, uint32_t *sum_32_pass) {
        for (int j = 0; j < 32; j++) {
            if (mem_tmp_1[j] == 0x42000000) {
                sum_32_pass[j] = 0xF0;  // pass
            } else {
                sum_32_pass[j] = 0x01;  // fail
            }
        }
}

void setup_correct_metadata() {
    printf("\n=== Setting up metadata with kernel args ===\n");
    
    // 1. 先清空整个metadata区域
    printf("Clearing metadata area...\n");
    for(int i = 0; i < 64; i++) {
        Xil_Out32(0x90024000 + i*4, 0);
    }
    
    // 2. 写入kernel参数到开头
    printf("Writing kernel args to metadata...\n");
    Xil_Out32(0x90024000, 0x90000000);  // input1
    Xil_Out32(0x90024004, 0x90001000);  // input2  
    Xil_Out32(0x90024008, 0x90002000);  // output
    
    // 3. 验证写入
    printf("Verifying metadata writes:\n");
    uint32_t val0 = Xil_In32(0x90024000);
    uint32_t val1 = Xil_In32(0x90024004);
    uint32_t val2 = Xil_In32(0x90024008);
    
    printf("  [0x00]: 0x%08x %s\n", val0, (val0 == 0x90000000) ? "✓" : "✗");
    printf("  [0x04]: 0x%08x %s\n", val1, (val1 == 0x90001000) ? "✓" : "✗");
    printf("  [0x08]: 0x%08x %s\n", val2, (val2 == 0x90002000) ? "✓" : "✗");
    
    if(val0 != 0x90000000 || val1 != 0x90001000 || val2 != 0x90002000) {
        printf("ERROR: Metadata write failed!\n");
        // 尝试再次写入
        printf("Retrying metadata write...\n");
        Xil_Out32(0x90024000, 0x90000000);
        Xil_Out32(0x90024004, 0x90001000);
        Xil_Out32(0x90024008, 0x90002000);
        
        // 再次验证
        val0 = Xil_In32(0x90024000);
        val1 = Xil_In32(0x90024004);
        val2 = Xil_In32(0x90024008);
        printf("After retry:\n");
        printf("  [0x00]: 0x%08x\n", val0);
        printf("  [0x04]: 0x%08x\n", val1);
        printf("  [0x08]: 0x%08x\n", val2);
    }
}
void patch_kernel_complete() {
    printf("\n=== Patching kernel for all parameters ===\n");
    
    // 读取三个参数
    Xil_Out32(0x80000060, 0x0002a303);  // lw t1, 0(t0)  - input1
    Xil_Out32(0x80000064, 0x0042a503);  // lw a0, 4(t0)  - input2
    Xil_Out32(0x80000068, 0x0082a383);  // lw t2, 8(t0)  - output
    
    // 验证
    printf("Patched instructions:\n");
    printf("  [0x60]: 0x%08x (lw t1, 0(t0))\n", Xil_In32(0x80000060));
    printf("  [0x64]: 0x%08x (lw a0, 4(t0))\n", Xil_In32(0x80000064));
    printf("  [0x68]: 0x%08x (lw t2, 8(t0))\n", Xil_In32(0x80000068));
}

void verify_input_data() {
    printf("\n=== Verifying Input Data ===\n");
    
    // 检查input1 (应该全是16.0 = 0x41800000)
    printf("Input1 at 0x90000000:\n");
    for(int i = 0; i < 4; i++) {
        uint32_t val = Xil_In32(0x90000000 + i*4);
        
        printf("  [%d]: 0x%08x  %s\n", i, val, 
               (val == 0x41800000) ? "✓" : "✗");
    }
    
    // 检查input2 (应该全是16.0 = 0x41800000)  
    printf("Input2 at 0x90001000:\n");
    for(int i = 0; i < 4; i++) {
        uint32_t val = Xil_In32(0x90001000 + i*4);
       
        printf("  [%d]: 0x%08x  %s\n", i, val, 
               (val == 0x41800000) ? "✓" : "✗");
    }
}
void analyze_kernel_store() {
    printf("\n=== Analyzing store instructions ===\n");
    
    // 查看0x80附近的指令
    for(int i = 0x70; i <= 0x90; i += 4) {
        uint32_t inst = Xil_In32(0x80000000 + i);
        printf("[0x%02x]: 0x%08x", i, inst);
        
        // 检查是否是store指令
        if((inst & 0x7f) == 0x23) {  // store opcode
            printf(" <- STORE instruction");
        }
        printf("\n");
    }
}
uint32_t inst1[256] = {
//     0x00000013, 
// 0x900004b7,  // lui   x9,  0x90000
// 0x0004a283,  // lw    x5,  0(x9)
// 0x00100413,  // addi  x8,  x0, 1
// 0x00541433,  // sll   x8,  x8, x5
// 0x90000537,  // lui   x10, 0x90000
// 0x10050513,  // addi  x10, x10, 0x100
// 0x900005b7,  // lui   x11, 0x90000
// 0x20058593,  // addi  x11, x11, 0x200
// 0x807024F3, csrrs	x9,pds,x0
// 0x0004A283, lw	x5,0(x9)
// 0x00100413, addi	x8,x0,1
// 0x00541433, sll	x8,x8,x5 x8=8
// 0x08048493, addi	x9,x9,128 # baseAddr of matM
// 0x028405B3, mul	x11,x8,x8 x11=64
// 0x00259593, slli	x11,x11,0x2 # sizeof(matrix) 
// 0x00B48533, add	x10,x9,x11 # baseAddr of matA
// 0x00B505B3, add	x11,x10,x11  # baseAddr of vecB
0x00000013, 
0x00000013, 
0x90000537, // lui x10,0x90000
0x10050513, //addi x10,x10,0x100
0x900004b7, // lui x9,0x90000
0x20048493, //addi  x9,  x9,  0x200
0x900005b7, //lui x11,0x90000
0x30058593, //addi  x11, x11, 0x300
0x00800413, //addi x8,x0,8
    0x5208a3d7,  // 0x24: vid.v v7
    0x800023f3,  // 0x28: csrrs x7,tid,x0
    0x0273c157,  // 0x2c: vadd.vx v2,v7,x7
    0xa222c1d7,  // 0x30: vsrl.vx v3,v2,x5
    0xfff40413,  // 0x34: addi x8,x8,-1
    0x26244257,  // 0x38: vand.vx v4,v2,x8
    0x00140413,  // 0x3c: addi x8,x8,1
    0x00000393,  // 0x40: addi x7,x0,0
    0x5e004fd7,  // 0x44: vmv.v.x v31,x0
    
    // Kernel1
    0x00138a13,  // 0x48: addi x20,x7,1
    // 0x00038a13, // 0x48: addi x20,x7,0
    0x024a4a57,  // 0x4c: vadd.vx v20,v4,x20
    0x5e044ad7,  // 0x50: vmv.v.x v21,x8
    // 0x054ad45b,  // 0x54: vbge v20,v21,9c  vbge v20,v21,9c这个是搞反了v20和v21吧
    // ac45b就是blt
    // 0x055A545B,  // vbge v20,v21,9c真正的
    0x055A445B,// vblt v20,v21,9c
    0x043f905b,  // 0x58: vbne v3,v31,98
    0x97446a57,  // 0x5c: vmul.vx v20,v20,x8
    0x00239893,  // 0x60: slli x17,x7,0x2
    0x02888a33,  // 0x64: mul x20,x17,x8
    0x011a0a33,  // 0x68: add x20,x20,x17
    0x00241813,  // 0x6c: slli x16,x8,0x2
    0x010a07b3,  // 0x70: add x15,x20,x16
    0x00a787b3,  // 0x74: add x15,x15,x10
    0x0b07e507,  // 0x78: vlse32.v v10,(x15),x16
    0x00aa07b3,  // 0x7c: add x15,x20,x10
    0x0007a783,  // 0x80: lw x15,0(x15)
    0x5e07c5d7,  // 0x84: vmv.v.x v11,x15
    0x82a59557,  // 0x88: vfdiv.vv v10,v10,v11
    0x010a07b3,  // 0x8c: add x15,x20,x16
    0x009787b3,  // 0x90: add x15,x15,x9
    0x0b07e527,  // 0x94: vsse32.v v10,(x15),x16
    
    // K1_END
    0x0000325b,  // 0x98: join v0,v0,9c
    
    // K1_A
    0x0000325b,  // 0x9c: join v0,v0,a0
    
    // K1_B
    0x0400400b,  // 0xa0: barrier x0,x0,0
    
    // Kernel2
    0x00138a13,  // 0xa4: addi x20,x7,1
    0x023a4a57,  // 0xa8: vadd.vx v20,v3,x20
    0x0243cad7,  // 0xac: vadd.vx v21,v4,x7
    0x5e044b57,  // 0xb0: vmv.v.x v22,x8
    0x074b5a5b,  // 0xb4: vbge v20,v22,128
    0x075b565b,  // 0xb8: vbge v21,v22,124
    // 0x076a4a5b,//可能会错
    // 0x076a465b,//可能会错
    0x97446957,  // 0xbc: vmul.vx v18,v20,x8
    0x0323c9d7,  // 0xc0: vadd.vx v19,v18,x7
    0x973139d7,  // 0xc4: vsll.vi v19,v19,2
    0x0f34e487,  // 0xc8: vloxei32.v v9,(x9),v19 向量索引加载（indexed vector load） 指令
    0x433027d7,  // 0xcc: vmv.x.s x15,v19 向量到标量移动 指令。
    0x00a787b3,  // 0xd0: add x15,x15,x10
    0x0207e507,  // 0xd4: vle32.v v10,(x15)
    0x02838a33,  // 0xd8: mul x20,x7,x8
    0x035a4b57,  // 0xdc: vadd.vx v22,v21,x20
    0x97613b57,  // 0xe0: vsll.vi v22,v22,2
    0x0f656607,  // 0xe4: vloxei32.v v12,(x10),v22
    0xbec49557,  // 0xe8: vfnmsac.vv v10,v9,v12
    0x0207e527,  // 0xec: vse32.v v10,(x15)
    0x023f985b,  // 0xf0: vbne v3,v31,120
    0x0350bad7,  // 0xf4: vadd.vi v21,v21,1
    0x5e044b57,  // 0xf8: vmv.v.x v22,x8
    0x035b505b,  // 0xfc: vbge v21,v22,11c
    0x00239313,  // 0x100: slli x6,x7,0x2
    0x00b30333,  // 0x104: add x6,x6,x11
    0x00032703,  // 0x108: lw x14,0(x6)
    0x00430313,  // 0x10c: addi x6,x6,4
    0x02036587,  // 0x110: vle32.v v11,(x6)
    0xbe9755d7,  // 0x114: vfnmsac.vf v11,f14,v9
    0x020365a7,  // 0x118: vse32.v v11,(x6)
    
    // K2_J0
    0x0000325b,  // 0x11c: join v0,v0,120
    
    // K2_END
    0x0000325b,  // 0x120: join v0,v0,124
    
    // K2_J1
    0x0000325b,  // 0x124: join v0,v0,128
    
    // K2_J2
    0x0000325b,  // 0x128: join v0,v0,12c
    
    // K2_B
    0x0400400b,  // 0x12c: barrier x0,x0,0
    0x00138393,  // 0x130: addi x7,x7,1
    0xf083cae3,  // 0x134: blt x7,x8,48
    0x0000400b,  // 0x138: endprg x0,x0,x0
    0x00008067,  // 0x13c: jalr x0,0(x1)
    
    // Padding with NOPs
    0x00000013,  // 0x140: nop
    0x00000013,  // 0x144: nop
    0x00000013,  // 0x148: nop
    0x00000013,  // 0x14c: nop
    0x00000013,  // 0x150: nop
    0x00000013,  // 0x154: nop
    0x00000013,  // 0x158: nop
    0x00000013,  // 0x15c: nop
    0x00000013,  // 0x160: nop
    0x00000013,  // 0x164: nop
    0x00000013,  // 0x168: nop
    0x00000013,  // 0x16c: nop
    0x00000013,  // 0x170: nop
    0x00000013,  // 0x174: nop
    0x00000013,  // 0x178: nop
    0x00000013,  // 0x17c: nop
    0x00000013,  // 0x180: nop
    0x00000013,  // 0x184: nop
    0x00000013,  // 0x188: nop
    0x00000013,  // 0x18c: nop
    0x00000013,  // 0x190: nop
    0x00000013,  // 0x194: nop
    0x00000013,  // 0x198: nop
    0x00000013,  // 0x19c: nop
    0x00000013,  // 0x1a0: nop
    0x00000013,  // 0x1a4: nop
    0x00000013,  // 0x1a8: nop
    0x00000013,  // 0x1ac: nop
    0x00000013,  // 0x1b0: nop
    0x00000013,  // 0x1b4: nop
    0x00000013,  // 0x1b8: nop
    0x00000013,  // 0x1bc: nop
    0x00000013,  // 0x1c0: nop
    0x00000013,  // 0x1c4: nop
    0x00000013,  // 0x1c8: nop
    0x00000013,  // 0x1cc: nop
    0x00000013,  // 0x1d0: nop
    0x00000013,  // 0x1d4: nop
    0x00000013,  // 0x1d8: nop
    0x00000013,  // 0x1dc: nop
    0x00000013,  // 0x1e0: nop
    0x00000013,  // 0x1e4: nop
    0x00000013,  // 0x1e8: nop
    0x00000013,  // 0x1ec: nop
    0x00000013,  // 0x1f0: nop
    0x00000013,  // 0x1f4: nop
    0x00000013,  // 0x1f8: nop
    0x00000013,  // 0x1fc: nop
    0x00000013,  // 0x200: nop
    0x00000013,  // 0x204: nop
    0x00000013,  // 0x208: nop
    0x00000013,  // 0x20c: nop
    0x00000013,  // 0x210: nop
    0x00000013,  // 0x214: nop
    0x00000013,  // 0x218: nop
    0x00000013,  // 0x21c: nop
    0x00000013,  // 0x220: nop
    0x00000013,  // 0x224: nop
    0x00000013,  // 0x228: nop
    0x00000013,  // 0x22c: nop
    0x00000013,  // 0x230: nop
    0x00000013,  // 0x234: nop
    0x00000013,  // 0x238: nop
    0x00000013,  // 0x23c: nop
    0x00000013,  // 0x240: nop
    0x00000013,  // 0x244: nop
    0x00000013,  // 0x248: nop
    0x00000013,  // 0x24c: nop
    0x00000013,  // 0x250: nop
    0x00000013,  // 0x254: nop
    0x00000013,  // 0x258: nop
    0x00000013,  // 0x25c: nop
    0x00000013,  // 0x260: nop
    0x00000013,  // 0x264: nop
    0x00000013,  // 0x268: nop
    0x00000013,  // 0x26c: nop
    0x00000013,  // 0x270: nop
    0x00000013,  // 0x274: nop
    0x00000013,  // 0x278: nop
    0x00000013,  // 0x27c: nop
    0x00000013,  // 0x280: nop
    0x00000013,  // 0x284: nop
    0x00000013,  // 0x288: nop
    0x00000013,  // 0x28c: nop
    0x00000013,  // 0x290: nop
    0x00000013,  // 0x294: nop
    0x00000013,  // 0x298: nop
    0x00000013,  // 0x29c: nop
    0x00000013,  // 0x2a0: nop
    0x00000013,  // 0x2a4: nop
    0x00000013,  // 0x2a8: nop
    0x00000013,  // 0x2ac: nop
    0x00000013,  // 0x2b0: nop
    0x00000013,  // 0x2b4: nop
    0x00000013,  // 0x2b8: nop
    0x00000013,  // 0x2bc: nop
    0x00000013,  // 0x2c0: nop
    0x00000013,  // 0x2c4: nop
    0x00000013,  // 0x2c8: nop
    0x00000013,  // 0x2cc: nop
    0x00000013,  // 0x2d0: nop
    0x00000013,  // 0x2d4: nop
    0x00000013,  // 0x2d8: nop
    0x00000013,  // 0x2dc: nop
    0x00000013,  // 0x2e0: nop
    0x00000013,  // 0x2e4: nop
    0x00000013,  // 0x2e8: nop
    0x00000013,  // 0x2ec: nop
    0x00000013,  // 0x2f0: nop
    0x00000013,  // 0x2f4: nop
    0x00000013,  // 0x2f8: nop
    0x00000013,  // 0x2fc: nop
    0x00000013,  // 0x300: nop
    0x00000013,  // 0x304: nop
    0x00000013,  // 0x308: nop
    0x00000013,  // 0x30c: nop
    0x00000013,  // 0x310: nop
    0x00000013,  // 0x314: nop
    0x00000013,  // 0x318: nop
    0x00000013,  // 0x31c: nop
    0x00000013,  // 0x320: nop
    0x00000013,  // 0x324: nop
    0x00000013,  // 0x328: nop
    0x00000013,  // 0x32c: nop
    0x00000013,  // 0x330: nop
    0x00000013,  // 0x334: nop
    0x00000013,  // 0x338: nop
    0x00000013,  // 0x33c: nop
    0x00000013,  // 0x340: nop
    0x00000013,  // 0x344: nop
    0x00000013,  // 0x348: nop
    0x00000013,  // 0x34c: nop
    0x00000013,  // 0x350: nop
    0x00000013,  // 0x354: nop
    0x00000013,  // 0x358: nop
    0x00000013,  // 0x35c: nop
    0x00000013,  // 0x360: nop
    0x00000013,  // 0x364: nop
    0x00000013,  // 0x368: nop
    0x00000013,  // 0x36c: nop
    0x00000013,  // 0x370: nop
    0x00000013,  // 0x374: nop
    0x00000013,  // 0x378: nop
    0x00000013,  // 0x37c: nop
    0x00000013,  // 0x380: nop
    0x00000013,  // 0x384: nop
    0x00000013,  // 0x388: nop
    0x00000013,  // 0x38c: nop
    0x00000013,  // 0x390: nop
    0x00000013,  // 0x394: nop
    0x00000013,  // 0x398: nop
    0x00000013,  // 0x39c: nop
    0x00000013,  // 0x3a0: nop
    0x00000013,  // 0x3a4: nop
    0x00000013,  // 0x3a8: nop
    0x00000013,  // 0x3ac: nop
    0x00000013,  // 0x3b0: nop
    0x00000013,  // 0x3b4: nop
    0x00000013,  // 0x3b8: nop
    0x00000013,  // 0x3bc: nop
    0x00000013,  // 0x3c0: nop
    0x00000013,  // 0x3c4: nop
    0x00000013,  // 0x3c8: nop
    0x00000013,  // 0x3cc: nop
};

void write_instructions() {
    // printf("Writing instructions to 0x80000000...\n");
    uint32_t inst_addr = 0x80000000;
    
    // 写入所有指令
    for (int i = 0; i < 256; i++) {
        Xil_Out32(inst_addr + i * 4, inst[i]);
    }
    
    // 验证写入
    // printf("Verifying instructions...\n");
    // for (int i = 0; i < 10; i++) {
    //     uint32_t val = Xil_In32(inst_addr + i * 4);
    //     printf("  [0x%08x] = 0x%08x (expected 0x%08x)\n", 
    //            inst_addr + i * 4, val, inst[i]);
    // }
}


void check_gaussian_results(int k) {
    // int n = 1 << k;  // n = 8
    // printf("\n=== Checking Gaussian results for k=%d, n=%d ===\n", k, n);
    printf("\n0x90000080:\n");
    for(int i = 0; i < 10; i++) {
        uint32_t val = Xil_In32(0x90000080 + i*4);
            float f;
            memcpy(&f, &val, sizeof(f));
        printf("  [0x%x] = 0x%08x (%.4f)\n", 0x100+i*4, val, f);
    }
    printf("\n0x90000100:\n");
    for(int i = 0; i < 10; i++) {
        uint32_t val = Xil_In32(0x90000100 + i*4);
            float f;
            memcpy(&f, &val, sizeof(f));
        printf("  [0x%x] = 0x%08x (%.4f)\n", 0x100+i*4, val, f);
    }
    printf("\n0x90000180:\n");
    for(int i = 0; i < 10; i++) {
        uint32_t val = Xil_In32(0x90000180 + i*4);
            float f;
            memcpy(&f, &val, sizeof(f));
        printf("  [0x%x] = 0x%08x (%.4f)\n", 0x180+i*4, val, f);
    }

}
void verify_instructions() {
    printf("First 20 instructions:\n");
    for(int i = 0; i < 20; i++) {
        uint32_t instr = Xil_In32(0x80000000 + i*4);
        printf("  [%x] 0x%08x\n", 0x80000000 + i*4, instr);
    }
}
void verify_memory_setup() {
    
    // 检查矩阵A的起始
    printf("\nMatrix A at 0x90000000:\n");
    for(int i = 0; i < 100; i++) {
        uint32_t val = Xil_In32(0x90000000 + i*4);
            float f;
            memcpy(&f, &val, sizeof(f));
        printf("  [0x%x] = 0x%08x (%.4f)\n", i*4, val, f);
    }



    
}