#include "ddac.h"

// 外部变量声明
extern DAC_HandleTypeDef hdac1;
extern TIM_HandleTypeDef htim2;
extern DMA_HandleTypeDef hdma_dac1_ch1;

DDS_HandleTypeDef dds_handler;

void DDS_Init(DDS_HandleTypeDef *hdds)
{
    // 初始化DDS参数
    hdds->phase_accumulator = 0;
    hdds->phase_increment = 0;
    hdds->output_enabled = 0;
	hdds->dma_buffer_size = WAVE_TABLE_SIZE;   
	
	// 生成波形表[6](@ref)
    DDS_GenerateWaveTable(hdds);
		
	// 设置默认频率
    DDS_SetFrequency(hdds, DEFAULT_FREQUENCY);
	 
    // 初始化DMA缓冲区
    memset(hdds->dma_buffer, 0, hdds->dma_buffer_size * sizeof(uint16_t));
}

void DDS_GenerateWaveTable(DDS_HandleTypeDef *hdds)
{
	for(int i=0; i<WAVE_TABLE_SIZE; i++) 
    {
      hdds->wave_table[i] = 2048 + 2047 * sin(2 * 3.1415926 * i / WAVE_TABLE_SIZE); // 12位， 0-3.3V范围  
    }
}



void DDS_Start(DDS_HandleTypeDef *hdds)
{
    //设置相位
	DDS_SetInitialPhase(hdds,change_phase);   
    
    // 更新DMA缓冲区
    DDS_UpdateBuffer(hdds, hdds->dma_buffer, hdds->dma_buffer_size);
    	
	// 启动定时器触发
    HAL_TIM_Base_Start(&htim2);
	
	HAL_Delay(1);
    // 启动DAC和DMA传输[7](@ref)
    HAL_DAC_Start_DMA(&hdac1, DAC_CHANNEL_1,(uint32_t*)hdds->dma_buffer,hdds->dma_buffer_size, DAC_ALIGN_12B_R);
     
    hdds->output_enabled = 1;
}


void DDS_REStart(DDS_HandleTypeDef *hdds)
{
 	HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_1);
    //复位控制字
	hdds->phase_accumulator = 0;
	//设置相位
	DDS_SetInitialPhase(hdds,85);
    // 更新DMA缓冲区
    DDS_UpdateBuffer(hdds, hdds->dma_buffer, hdds->dma_buffer_size);
	//重启传输
	HAL_DAC_Start_DMA(&hdac1, DAC_CHANNEL_1,(uint32_t*)hdds->dma_buffer,hdds->dma_buffer_size, DAC_ALIGN_12B_R);

    hdds->output_enabled = 1;
}

void DDS_Stop(DDS_HandleTypeDef *hdds)
{
    // 停止DAC和定时器
    HAL_DAC_Stop_DMA(&hdac1, DAC_CHANNEL_1);
    HAL_TIM_Base_Stop(&htim2);
    
    hdds->output_enabled = 0;
}

void DDS_SetFrequency(DDS_HandleTypeDef *hdds, float frequency)
{
    // 计算相位增量[4](@ref)
    // 公式: phase_increment = (f_out * 2^32) / f_sampling
    uint32_t sampling_rate = SAMPLE_RATE;
    
    if(frequency > sampling_rate / 2) {
        frequency = sampling_rate / 2; // 限制在奈奎斯特频率内
    }
    
    hdds->phase_increment = (uint32_t)((frequency * 4294967296.0) / sampling_rate);
}

uint8_t change_phase;


//调用此函数更新数据，引入句柄是为了引入已知数据，填充的数组可以自定义。
void DDS_UpdateBuffer(DDS_HandleTypeDef *hdds, uint16_t *buffer, uint32_t size)
{
    // 使用DDS算法填充缓冲区[4](@ref)
//	    DDS_SetInitialPhase(hdds,change_phase);
	 for(int i = 0; i < size; i++) {
        // 1. 更新相位累加器
        hdds->phase_accumulator += hdds->phase_increment;
        
        // 2. 加上相位偏移
        uint32_t phase_with_offset = hdds->phase_accumulator + hdds->phase_offset;
        
        // 3. 取相位累加器的高9位作为波形表索引
        uint16_t index = (phase_with_offset >> 23) & 0x0FFF;
        
        // 4. 从波形表读取数据
        buffer[i] = hdds->wave_table[index];
    }
}

// DMA传输完成回调函数[4](@ref)
void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef *hdac)
{
    // 当DMA传输完成时，更新缓冲区
    if(dds_handler.output_enabled == 1&& hdac->Instance == DAC1) {
        DDS_UpdateBuffer(&dds_handler, dds_handler.dma_buffer, dds_handler.dma_buffer_size);
    }
}

void DDS_SetInitialPhase(DDS_HandleTypeDef *hdds, float phase_degrees)
{
    // 将角度转换为弧度
    float phase_rad = phase_degrees * 3.1415926f / 180.0f;
    
    // 将相位转换为相位累加器值
    // 相位范围为0-2π，对应相位累加器值0-2^32
    hdds->phase_offset = (uint32_t)(phase_rad * 4294967296.0f / (2 * 3.1415926f));
}
