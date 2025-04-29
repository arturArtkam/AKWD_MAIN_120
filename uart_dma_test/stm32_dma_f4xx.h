#ifndef STM32TPL_STM32_DMA_F4XX_H_INCLUDED
#define STM32TPL_STM32_DMA_F4XX_H_INCLUDED

//#include <cstddef>

//namespace STM32
//{
//
//namespace DMA
//{

/*
 * Enumeration for all DMA Streams
 */
enum DmaChannelNum
{
	DMA1_CH0,
	DMA1_CH1,
	DMA1_CH2,
	DMA1_CH3,
	DMA1_CH4,
	DMA1_CH5,
	DMA1_CH6,
	DMA1_CH7,

	DMA2_CH0,
	DMA2_CH1,
	DMA2_CH2,
	DMA2_CH3,
	DMA2_CH4,
	DMA2_CH5,
	DMA2_CH6,
	DMA2_CH7
};

namespace
{
/*
 * DMA traits. Used internally.
 * Declares some constants, based on DMA number.
 */
template <int dmaNo> struct dma_traits;

template<>
struct dma_traits<1>
{
	enum
	{
		DMA_NO = 1,
		DMAx_BASE = DMA1_BASE,
		RCC_AHBENR_DMAxEN = RCC_AHB1ENR_DMA1EN
	};
};

template<>
struct dma_traits<2>
{
	enum
	{
		DMA_NO = 2,
		DMAx_BASE = DMA2_BASE,
		RCC_AHBENR_DMAxEN = RCC_AHB1ENR_DMA2EN
	};
};

/*
 * DMA stream traits. Used internally.
 * Declares some constants, based on DMA stream number.
 */
template<DmaChannelNum chNum> struct dma_stream_traits;

template<>
struct dma_stream_traits<DMA1_CH0>
{
	static const IRQn DMAChannel_IRQn = DMA1_Stream0_IRQn;
	enum
	{
		DMA_NO = 1,
		CHANNEL_NO = 0,
		MASK_SHIFT = 0
	};
};

template<>
struct dma_stream_traits<DMA1_CH1>
{
	static const IRQn DMAChannel_IRQn = DMA1_Stream1_IRQn;
	enum
	{
		DMA_NO = 1,
		CHANNEL_NO = 1,
		MASK_SHIFT = 6
	};
};

template<>
struct dma_stream_traits<DMA1_CH2>
{
	static const IRQn DMAChannel_IRQn = DMA1_Stream2_IRQn;
	enum
	{
		DMA_NO = 1,
		CHANNEL_NO = 2,
		MASK_SHIFT = 16
	};
};

template<>
struct dma_stream_traits<DMA1_CH3>
{
	static const IRQn DMAChannel_IRQn = DMA1_Stream3_IRQn;
	enum
	{
		DMA_NO = 1,
		CHANNEL_NO = 3,
		MASK_SHIFT = 22
	};
};

template<>
struct dma_stream_traits<DMA1_CH4>
{
	static const IRQn DMAChannel_IRQn = DMA1_Stream4_IRQn;
	enum
	{
		DMA_NO = 1,
		CHANNEL_NO = 4,
		MASK_SHIFT = 0
	};
};

template<>
struct dma_stream_traits<DMA1_CH5>
{
	static const IRQn DMAChannel_IRQn = DMA1_Stream5_IRQn;
	enum
	{
		DMA_NO = 1,
		CHANNEL_NO = 5,
		MASK_SHIFT = 6
	};
};

template<>
struct dma_stream_traits<DMA1_CH6>
{
	static const IRQn DMAChannel_IRQn = DMA1_Stream6_IRQn;
	enum
	{
		DMA_NO = 1,
		CHANNEL_NO = 6,
		MASK_SHIFT = 16
	};
};

template<>
struct dma_stream_traits<DMA1_CH7>
{
	static const IRQn DMAChannel_IRQn = DMA1_Stream7_IRQn;
	enum
	{
		DMA_NO = 1,
		CHANNEL_NO = 7,
		MASK_SHIFT = 22
	};
};

template<>
struct dma_stream_traits<DMA2_CH0>
{
	static const IRQn DMAChannel_IRQn = DMA2_Stream0_IRQn;
	enum
	{
		DMA_NO = 2,
		CHANNEL_NO = 0,
		MASK_SHIFT = 0
	};
};

template<>
struct dma_stream_traits<DMA2_CH1>
{
	static const IRQn DMAChannel_IRQn = DMA2_Stream1_IRQn;
	enum
	{
		DMA_NO = 2,
		CHANNEL_NO = 1,
		MASK_SHIFT = 6
	};
};

template<>
struct dma_stream_traits<DMA2_CH2>
{
	static const IRQn DMAChannel_IRQn = DMA2_Stream2_IRQn;
	enum
	{
		DMA_NO = 2,
		CHANNEL_NO = 2,
		MASK_SHIFT = 16
	};
};

template<>
struct dma_stream_traits<DMA2_CH3>
{
	static const IRQn DMAChannel_IRQn = DMA2_Stream3_IRQn;
	enum
	{
		DMA_NO = 2,
		CHANNEL_NO = 3,
		MASK_SHIFT = 22
	};
};

template<>
struct dma_stream_traits<DMA2_CH4>
{
	static const IRQn DMAChannel_IRQn = DMA2_Stream4_IRQn;
	enum
	{
		DMA_NO = 2,
		CHANNEL_NO = 4,
		MASK_SHIFT = 0
	};
};

template<>
struct dma_stream_traits<DMA2_CH5>
{
	static const IRQn DMAChannel_IRQn = DMA2_Stream5_IRQn;
	enum
	{
		DMA_NO = 2,
		CHANNEL_NO = 5,
		MASK_SHIFT = 6
	};
};

template<>
struct dma_stream_traits<DMA2_CH6>
{
	static const IRQn DMAChannel_IRQn = DMA2_Stream6_IRQn;
	enum
	{
		DMA_NO = 2,
		CHANNEL_NO = 6,
		MASK_SHIFT = 16
	};
};

template<>
struct dma_stream_traits<DMA2_CH7>
{
	static const IRQn DMAChannel_IRQn = DMA2_Stream7_IRQn;
	enum
	{
		DMA_NO = 2,
		CHANNEL_NO = 7,
		MASK_SHIFT = 22
	};
};

} /* anon namespace */

template <bool IsHi> struct lo_hi_selector {
	enum { OFFSET   = 0 };
};

template <> struct lo_hi_selector<true> {
	enum { OFFSET   = 1 };
};

template<DmaChannelNum ch_num>
class dma_stream
{
private:
	typedef dma_stream_traits<ch_num> stream_traits;

public:
	/* Stream-specific constants */
	static const IRQn DMAChannel_IRQn = stream_traits::DMAChannel_IRQn;
	enum
	{
		DMA_NO                   = stream_traits::DMA_NO,
		CHANNEL_NO               = stream_traits::CHANNEL_NO,
		DMAx_BASE                = dma_traits<DMA_NO>::DMAx_BASE,
		CHANNEL_BASE             = DMAx_BASE + 0x10 + 0x18 * CHANNEL_NO,
		RCC_AHBENR_DMAxEN        = dma_traits<DMA_NO>::RCC_AHBENR_DMAxEN,
        LO_HI_OFFSET             = lo_hi_selector<(CHANNEL_NO > 3)>::OFFSET
	};

	/* Stream-specific masks */
	enum
	{
		MASK_SHIFT       = stream_traits::MASK_SHIFT,
		DMA_MASK_ALL     = 0x3D << MASK_SHIFT,
		DMA_MASK_FEIF    = 0x01 << MASK_SHIFT,    // FIFO error
		DMA_MASK_DMEIF   = 0x04 << MASK_SHIFT,    // direct mode error
		DMA_MASK_TEIF    = 0x08 << MASK_SHIFT,    // transfer error
        DMA_MASK_CTEIF   = 0x08 << MASK_SHIFT,    // clear transfer error flag
		DMA_MASK_HTIF    = 0x10 << MASK_SHIFT,    // half transfer
		DMA_MASK_TCIF    = 0x20 << MASK_SHIFT,     // transfer complete
        DMA_MASK_CTCIF   = 0x20 << MASK_SHIFT     // clear transfer complete flag
	};

    struct dma_typedef_t
    {
      __IO uint32_t ISR[2];   /* DMA interrupt status register,      Address offset: 0x00 */
      __IO uint32_t IFCR[2];  /* DMA interrupt flag clear register, Address offset: 0x08 */
    };

	/// Constructor.
	dma_stream() {}

	static void enable_clocks()   { RCC->AHB1ENR |= RCC_AHBENR_DMAxEN;  __DSB(); }
	static void disable_clocks()  { RCC->AHB1ENR &= ~RCC_AHBENR_DMAxEN; __DSB(); }
	static void enable()          { stream->CR |= DMA_SxCR_EN; }
	static void disable()         { stream->CR &= ~DMA_SxCR_EN; }
	static bool enabled()         { return stream->CR & DMA_SxCR_EN; }

    //static constexpr DMA_TypeDef* DMAn = ((DMA_TypeDef* ) DMAx_BASE);
    static constexpr dma_typedef_t* DMAn = (reinterpret_cast<dma_typedef_t* >(DMAx_BASE));
	static constexpr DMA_Stream_TypeDef* stream = ((DMA_Stream_TypeDef* ) CHANNEL_BASE);
};

typedef dma_stream<DMA1_CH0> Dma1Channel0;
typedef dma_stream<DMA1_CH1> Dma1Channel1;
typedef dma_stream<DMA1_CH2> Dma1Channel2;
typedef dma_stream<DMA1_CH3> Dma1Channel3;
typedef dma_stream<DMA1_CH4> Dma1Channel4;
typedef dma_stream<DMA1_CH5> Dma1Channel5;
typedef dma_stream<DMA1_CH6> Dma1Channel6;
typedef dma_stream<DMA1_CH7> Dma1Channel7;

typedef dma_stream<DMA2_CH0> Dma2Channel0;
typedef dma_stream<DMA2_CH1> Dma2Channel1;
typedef dma_stream<DMA2_CH2> Dma2Channel2;
typedef dma_stream<DMA2_CH3> Dma2Channel3;
typedef dma_stream<DMA2_CH4> Dma2Channel4;
typedef dma_stream<DMA2_CH5> Dma2Channel5;
typedef dma_stream<DMA2_CH6> Dma2Channel6;
typedef dma_stream<DMA2_CH7> Dma2Channel7;
//
//}  // namespace DMA
//
//}  // namespace STM32

#endif // STM32TPL_STM32_DMA_F4XX_H_INCLUDED
