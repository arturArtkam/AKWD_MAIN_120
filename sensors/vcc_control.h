#ifndef VCC_CONTROL_H
#define VCC_CONTROL_H

class Vcc
{
    enum
    {
        VREF_MV = 3340,
        ADC_RESOLUTION = 12
    };

public:
    static void io_init()
    {
        typedef pin<PORTC, 0, GPIO_Mode_AN> Pos_5v_adc_channel;
        typedef pin<PORTC, 2, GPIO_Mode_AN> Neg_5v_adc_channel;

        Pos_5v_adc_channel::init();
        Neg_5v_adc_channel::init();
    }

    static void adc_init()
    {
        ADC_InitTypeDef adc;

        RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

        ADC_StructInit(&adc);
        adc.ADC_Resolution = ADC_Resolution_12b;
        adc.ADC_ScanConvMode = DISABLE;
        adc.ADC_ContinuousConvMode = DISABLE;
        adc.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
        adc.ADC_DataAlign = ADC_DataAlign_Right;
        ADC_Init(ADC1, &adc);

        ADC_Cmd(ADC1, ENABLE);
    }

    static uint16_t adc_read_channel(uint8_t channel)
    {
        ADC_RegularChannelConfig(ADC1, channel, 1, ADC_SampleTime_480Cycles);
        ADC_SoftwareStartConv(ADC1);
        while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));

        return ADC_GetConversionValue(ADC1);
    }

    static float read_pos_5v()
    {
        return (2.0f * float(adc_read_channel(ADC_Channel_10)) * float(float(VREF_MV) / 1000.0f)) / float(1 << ADC_RESOLUTION);
    }

    static float read_neg_5v()
    {
        return (-2.0f * float(adc_read_channel(ADC_Channel_12)) * float(float(VREF_MV) / 1000.0f)) / float(1 << ADC_RESOLUTION);
    }
};

#endif /* VCC_CONTROL_H */
