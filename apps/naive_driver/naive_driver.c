#include "naive_driver.h"
#include "stdio.h"

#include "metadata.h"
#include "data.h"
#include <klib.h>

typedef uint32_t u32;
typedef uintptr_t UINTPTR;
#define GPIO_DEVICE_ID     XPAR_GPIO_0_DEVICE_ID  // DEVICE ID
#define GPIO_LED_PIN       1  //  LED PIN


void fix_metadata_layout() {
    // 清空metadata区域
    for(int i = 0; i < 64; i++) {
        Xil_Out32(0x90024000 + i*4, 0);
    }
    
    // 将kernel参数放在开头
    Xil_Out32(0x90024000, 0x90000000);  // input1
    Xil_Out32(0x90024004, 0x90001000);  // input2  
    Xil_Out32(0x90024008, 0x90002000);  // output
    
    // 其他metadata放后面
    Xil_Out32(0x9002400c, 0x800000a8);
    Xil_Out32(0x90024010, 0x90003000);
    // ...
}
int main(){
    



    Gpu TestGpu;
    // TODO: Set GPU_BASEADDR in .h file
    GpuInit(&TestGpu, GPU_BASEADDR);
    TaskMemory TestMem;
    GpuTaskMemoryInit(&TestMem, 128, 1024);
    
    //host to cta
//    uint64_t noused;
//    uint64_t kernel_id;
//    uint64_t kernal_size0;
//    uint64_t kernal_size1;
//    uint64_t kernal_size2;
    uint64_t wf_size;
    uint64_t wg_size;
    uint64_t metaDataBaseAddr;
//    uint64_t ldsSize;
    uint64_t pdsSize;
    uint64_t sgprUsage;
    uint64_t vgprUsage;
    uint64_t pdsBaseAddr;
//      uint64_t num_buffer;
    uint32_t pds_size;
    //assign metadata
    void assign_metadata_values(uint32_t* Instr) {
//        noused = ((uint64_t)Instr[1] << 32) | (uint64_t)Instr[0];
//        kernel_id = ((uint64_t)Instr[3] << 32) | (uint64_t)Instr[2];
//        kernal_size0 = ((uint64_t)Instr[5] << 32) | (uint64_t)Instr[4];
//        kernal_size1 = ((uint64_t)Instr[7] << 32) | (uint64_t)Instr[6];
//        kernal_size2 = ((uint64_t)Instr[9] << 32) | (uint64_t)Instr[8];
        wf_size = ((uint64_t)Instr[11] << 32) | (uint64_t)Instr[10];
        wg_size = ((uint64_t)Instr[13] << 32) | (uint64_t)Instr[12];
        metaDataBaseAddr = ((uint64_t)Instr[15] << 32) | (uint64_t)Instr[14];
//        ldsSize = ((uint64_t)Instr[17] << 32) | (uint64_t)Instr[16];
        pdsSize = ((uint64_t)Instr[19] << 32) | (uint64_t)Instr[18];
        sgprUsage = ((uint64_t)Instr[21] << 32) | (uint64_t)Instr[20];
        vgprUsage = ((uint64_t)Instr[23] << 32) | (uint64_t)Instr[22];
        pdsBaseAddr = ((uint64_t)Instr[25] << 32) | (uint64_t)Instr[24];
//        num_buffer = ((uint64_t)Instr[27] << 32) | (uint64_t)Instr[26];
        pds_size = 0;
        // printf("wf_size         = 0x%lx (%lu)\n", wf_size, wf_size);
        // printf("wg_size         = 0x%lx (%lu)\n", wg_size, wg_size);
        // printf("metaDataBaseAddr= 0x%lx (%lu)\n", metaDataBaseAddr, metaDataBaseAddr);
        // printf("pdsSize         = 0x%lx (%lu)\n", pdsSize, pdsSize);
        // printf("sgprUsage       = 0x%lx (%lu)\n", sgprUsage, sgprUsage);
        // printf("vgprUsage       = 0x%lx (%lu)\n", vgprUsage, vgprUsage);
        // printf("pdsBaseAddr     = 0x%lx (%lu)\n", pdsBaseAddr, pdsBaseAddr);
    }
    assign_metadata_values(metadata);
    TaskConfig TestTask = {
            0,   // WgId
        (uint32_t)wg_size,   // NumWf
        (uint32_t)wf_size,   // WfSize
                0x80000000,   // StartPC
                (uint32_t)wg_size*(uint32_t)vgprUsage,   // VgprSizeTotal
                (uint32_t)wg_size*(uint32_t)sgprUsage,   // SgprSizeTotal
        128,   // LdsSize
                (uint32_t)vgprUsage,   // VgprSizePerWf
                (uint32_t)sgprUsage,   // SgprSizePerWf
                (uint32_t)metaDataBaseAddr,//metaDataBaseAddr
                (uint32_t)0x90000100,//(uint32_t)pdsBaseAddr+pds_size*(uint32_t)wf_size*(uint32_t)wg_size,//Host_req_pds_baseaddr
        TestMem // Mem
    };

    init_mem(metadata,data,128,1024);
    write_instructions();
    // verify_instructions();
    
    int k = data[0];
    printf("\nSAXPY %d\n", k);
    int k1 = data[1];
    uint32_t val = k1;
    float f;
    memcpy(&f, &val, sizeof(f));
    printf("SAXPY B[%d] = %.4f\n", k1, f);
    // verify_memory_setup();
    int retry = 5;
    // u32 WarpGroupID;
    while(retry){
        if(GpuSendTask(&TestGpu, &TestTask) == XST_SUCCESS)
            break;
        retry--;
    }
    // GpuDebugPrintRegs(&TestGpu);
    printf("GpuSendTask\n");
    if(!retry){
        printf("All tries failed. Stopped.\r\n");
        return XST_FAILURE;
    }


//     // TODO: Result Checking
check_gaussian_results(k);
// verify_memory_setup();
    // for (int j = 0; j < 32; j++) {
    //         printf("sum %lx\n",sum_32_pass[j]);
    //     }


//     // End of Checking
    GpuDeleteTask(&TestTask);
    // printf("GpuDeleteTask\n");

    return XST_SUCCESS;
}
