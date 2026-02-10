#ifndef __DDAC_H
#define __DDAC_H

#include "main.h"
#include "math.h"
#include "stdlib.h"
#include "string.h"

// DDS配置参数
#define WAVE_TABLE_SIZE 512   // 波形表大小
#define DEFAULT_FREQUENCY 1000  // 默认频率1kHz
#define SAMPLE_RATE 100000     // 采样率200kHz


// DDS控制结构体
typedef struct {
    uint16_t wave_table[WAVE_TABLE_SIZE];  // 波形数据表
    uint32_t phase_accumulator;            // 相位累加器(32位)
    uint32_t phase_increment;              // 相位增量(决定频率)
    uint16_t dma_buffer[WAVE_TABLE_SIZE];  // DMA缓冲区（静态）
    uint32_t dma_buffer_size;              // DMA缓冲区大小
    uint8_t output_enabled;                // 输出使能标志
	uint32_t phase_offset;                 //可动态调整的相位偏移量
} DDS_HandleTypeDef;

// 函数声明
void DDS_Init(DDS_HandleTypeDef *hdds);
void DDS_Start(DDS_HandleTypeDef *hdds);
void DDS_Stop(DDS_HandleTypeDef *hdds);
void DDS_SetFrequency(DDS_HandleTypeDef *hdds, float frequency);
void DDS_GenerateWaveTable(DDS_HandleTypeDef *hdds);
void DDS_UpdateBuffer(DDS_HandleTypeDef *hdds, uint16_t *buffer, uint32_t size);
void DDS_SetInitialPhase(DDS_HandleTypeDef *hdds, float phase_degrees);
void DDS_REStart(DDS_HandleTypeDef *hdds);
// 使用CORDIC硬件加速的正弦计算[6](@ref)
extern uint8_t change_phase;
extern DDS_HandleTypeDef dds_handler;
#endif
