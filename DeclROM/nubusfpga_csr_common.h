#ifndef __NUBUSFPGA_CSR_COMMON_H__
#define __NUBUSFPGA_CSR_COMMON_H__

/* from hw/common.h, +a32 */

#define CONFIG_CSR_DATA_WIDTH 32

/* CSR data width (subreg. width) in bytes, for direct comparson to sizeof() */
#define CSR_DW_BYTES     (CONFIG_CSR_DATA_WIDTH/8)
#define CSR_OFFSET_BYTES 4

/* Number of subregs required for various total byte sizes, by subreg width:
 * NOTE: 1, 2, 4, and 8 bytes represent uint[8|16|32|64]_t C types; However,
 *       CSRs of intermediate byte sizes (24, 40, 48, and 56) are NOT padded
 *       (with extra unallocated subregisters) to the next valid C type!
 *  +-----+-----------------+
 *  | csr |      bytes      |
 *  | _dw | 1 2 3 4 5 6 7 8 |
 *  |     |-----=---=-=-=---|
 *  |  1  | 1 2 3 4 5 6 7 8 |
 *  |  2  | 1 1 2 2 3 3 4 4 |
 *  |  4  | 1 1 1 1 2 2 2 2 |
 *  |  8  | 1 1 1 1 1 1 1 1 |
 *  +-----+-----------------+ */
static inline int num_subregs(int csr_bytes)
{
	return (csr_bytes - 1) / CSR_DW_BYTES + 1;
}

/* Read a CSR of size 'csr_bytes' located at address 'a'. */
static inline uint64_t _csr_rd(uint32_t a32, unsigned long a, int csr_bytes)
{
  uint64_t r = __builtin_bswap32(*((uint32_t*)(a32 + a)));
	for (int i = 1; i < num_subregs(csr_bytes); i++) {
		r <<= CONFIG_CSR_DATA_WIDTH;
		a += CSR_OFFSET_BYTES;
		r |= __builtin_bswap32(*((uint32_t*)(a32 + a)));
	}
	return r;
}

/* Write value 'v' to a CSR of size 'csr_bytes' located at address 'a'. */
static inline void _csr_wr(uint32_t a32, unsigned long a, uint64_t v, int csr_bytes)
{
	int ns = num_subregs(csr_bytes);
	for (int i = 0; i < ns; i++) {
	  *((uint32_t*)(a32 + a)) = __builtin_bswap32(v >> (CONFIG_CSR_DATA_WIDTH * (ns - 1 - i)));
		a += CSR_OFFSET_BYTES;
	}
}

// FIXME: - should we provide 24, 40, 48, and 56 bit csr_[rd|wr] methods?

static inline uint8_t csr_rd_uint8(uint32_t a32, unsigned long a)
{
	return _csr_rd(a32, a, sizeof(uint8_t));
}

static inline void csr_wr_uint8(uint32_t a32, uint8_t v, unsigned long a)
{
	_csr_wr(a32, a, v, sizeof(uint8_t));
}

static inline uint16_t csr_rd_uint16(uint32_t a32, unsigned long a)
{
	return _csr_rd(a32, a, sizeof(uint16_t));
}

static inline void csr_wr_uint16(uint32_t a32, uint16_t v, unsigned long a)
{
	_csr_wr(a32, a, v, sizeof(uint16_t));
}

static inline uint32_t csr_rd_uint32(uint32_t a32, unsigned long a)
{
	return _csr_rd(a32, a, sizeof(uint32_t));
}

static inline void csr_wr_uint32(uint32_t a32, uint32_t v, unsigned long a)
{
	_csr_wr(a32, a, v, sizeof(uint32_t));
}

static inline uint64_t csr_rd_uint64(uint32_t a32, unsigned long a)
{
	return _csr_rd(a32, a, sizeof(uint64_t));
}

static inline void csr_wr_uint64(uint32_t a32, uint64_t v, unsigned long a)
{
	_csr_wr(a32, a, v, sizeof(uint64_t));
}

/* Read a CSR located at address 'a' into an array 'buf' of 'cnt' elements.
 *
 * NOTE: Since CSR_DW_BYTES is a constant here, we might be tempted to further
 * optimize things by leaving out one or the other of the if() branches below,
 * depending on each unsigned type width;
 * However, this code is also meant to serve as a reference for how CSRs are
 * to be manipulated by other programs (e.g., an OS kernel), which may benefit
 * from dynamically handling multiple possible CSR subregister data widths
 * (e.g., by passing a value in through the Device Tree).
 * Ultimately, if CSR_DW_BYTES is indeed a constant, the compiler should be
 * able to determine on its own whether it can automatically optimize away one
 * of the if() branches! */
#define _csr_rd_buf(a32, a, buf, cnt)					\
  {									\
    int i, j, nsubs, n_sub_elem;					\
    uint64_t r;								\
    if (sizeof(buf[0]) >= CSR_DW_BYTES) {				\
      /* one or more subregisters per element */			\
      for (i = 0; i < cnt; i++) {					\
	buf[i] = _csr_rd(a32, a, sizeof(buf[0]));			\
	a += CSR_OFFSET_BYTES * num_subregs(sizeof(buf[0]));		\
      }									\
    } else {								\
      /* multiple elements per subregister (2, 4, or 8) */		\
      nsubs = num_subregs(sizeof(buf[0]) * cnt);			\
      n_sub_elem = CSR_DW_BYTES / sizeof(buf[0]);			\
      for (i = 0; i < nsubs; i++) {					\
	r = __builtin_bswap32(*(uint32_t*)(a32 + a));			\
      for (j = n_sub_elem - 1; j >= 0; j--) {				\
	if (i * n_sub_elem + j < cnt)					\
	  buf[i * n_sub_elem + j] = r;					\
	r >>= sizeof(buf[0]) * 8;					\
      }									\
      a += CSR_OFFSET_BYTES;						\
      }									\
    }									\
  }
/* Write an array 'buf' of 'cnt' elements to a CSR located at address 'a'.
 *
 * NOTE: The same optimization considerations apply here as with _csr_rd_buf()
 * above.
 */
#define _csr_wr_buf(a32, a, buf, cnt) \
{ \
	int i, j, nsubs, n_sub_elem; \
	uint64_t v; \
	if (sizeof(buf[0]) >= CSR_DW_BYTES) { \
		/* one or more subregisters per element */ \
		for (i = 0; i < cnt; i++) { \
			_csr_wr(a32, a, buf[i], sizeof(buf[0]));				 \
			a += CSR_OFFSET_BYTES * num_subregs(sizeof(buf[0])); \
		} \
	} else { \
		/* multiple elements per subregister (2, 4, or 8) */ \
		nsubs = num_subregs(sizeof(buf[0]) * cnt); \
		n_sub_elem = CSR_DW_BYTES / sizeof(buf[0]); \
		for (i = 0; i < nsubs; i++) { \
			v = buf[i * n_sub_elem + 0]; \
			for (j = 1; j < n_sub_elem; j++) { \
				if (i * n_sub_elem + j == cnt) \
					break; \
				v <<= sizeof(buf[0]) * 8; \
				v |= buf[i * n_sub_elem + j]; \
			} \
			*((uint32_t*)(a32 + a)) = __builtin_bswap32(v);	\
			a += CSR_OFFSET_BYTES;	\
		} \
	} \
}

static inline void csr_rd_buf_uint8(uint32_t a32, unsigned long a, uint8_t *buf, int cnt)
{
	_csr_rd_buf(a32, a, buf, cnt);
}

static inline void csr_wr_buf_uint8(uint32_t a32, unsigned long a,
					const uint8_t *buf, int cnt)
{
	_csr_wr_buf(a32, a, buf, cnt);
}

static inline void csr_rd_buf_uint16(uint32_t a32, unsigned long a, uint16_t *buf, int cnt)
{
	_csr_rd_buf(a32, a, buf, cnt);
}

static inline void csr_wr_buf_uint16(uint32_t a32, unsigned long a,
					const uint16_t *buf, int cnt)
{
	_csr_wr_buf(a32, a, buf, cnt);
}

static inline void csr_rd_buf_uint32(uint32_t a32, unsigned long a, uint32_t *buf, int cnt)
{
	_csr_rd_buf(a32, a, buf, cnt);
}

static inline void csr_wr_buf_uint32(uint32_t a32, unsigned long a,
					const uint32_t *buf, int cnt)
{
	_csr_wr_buf(a32, a, buf, cnt);
}

/* NOTE: the macros' "else" branch is unreachable, no need to be warned
 * about a >= 64bit left shift! */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshift-count-overflow"
static inline void csr_rd_buf_uint64(uint32_t a32, unsigned long a, uint64_t *buf, int cnt)
{
	_csr_rd_buf(a32, a, buf, cnt);
}

static inline void csr_wr_buf_uint64(uint32_t a32, unsigned long a,
					const uint64_t *buf, int cnt)
{
	_csr_wr_buf(a32, a, buf, cnt);
}
#pragma GCC diagnostic pop


#endif // __NUBUSFPGA_CSR_COMMON_H__
