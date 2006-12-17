/*
$Id$
    OWFS -- One-Wire filesystem
    OWHTTPD -- One-Wire Web Server
    Written 2003 Paul H Alfille
	email: palfille@earthlink.net
	Released under the GPL
	See the header file: ow.h for full attribution
	1wire/iButton system from Dallas Semiconductor
*/

/* General Device File format:
    This device file corresponds to a specific 1wire/iButton chip type
	( or a closely related family of chips )

	The connection to the larger program is through the "device" data structure,
	  which must be declared in the acompanying header file.

	The device structure holds the
	  family code,
	  name,
	  device type (chip, interface or pseudo)
	  number of properties,
	  list of property structures, called "filetype".

	Each filetype structure holds the
	  name,
	  estimated length (in bytes),
	  aggregate structure pointer,
	  data format,
	  read function,
	  write funtion,
	  generic data pointer

	The aggregate structure, is present for properties that several members
	(e.g. pages of memory or entries in a temperature log. It holds:
	  number of elements
	  whether the members are lettered or numbered
	  whether the elements are stored together and split, or separately and joined
*/

#include <config.h>
#include "owfs_config.h"
#include "ow_2505.h"

/* ------- Prototypes ----------- */

bREAD_FUNCTION(FS_r_page);
bWRITE_FUNCTION(FS_w_page);
bREAD_FUNCTION(FS_r_memory);
bWRITE_FUNCTION(FS_w_memory);

/* ------- Structures ----------- */

struct aggregate A2505 = { 64, ag_numbers, ag_separate, };
struct filetype DS2505[] = {
	F_STANDARD,
  {"memory", 2048, NULL, ft_binary, fc_stable, {b: FS_r_memory}, {b: FS_w_memory}, {v:NULL},},
  {"pages", 0, NULL, ft_subdir, fc_volatile, {v: NULL}, {v: NULL}, {v:NULL},},
  {"pages/page", 32, &A2505, ft_binary, fc_stable, {b: FS_r_page}, {b: FS_w_page}, {v:NULL},},
};

DeviceEntry(0 B, DS2505);

struct filetype DS1985U[] = {
	F_STANDARD,
  {"memory", 2048, NULL, ft_binary, fc_stable, {b: FS_r_memory}, {b: FS_w_memory}, {v:NULL},},
  {"pages", 0, NULL, ft_subdir, fc_volatile, {v: NULL}, {v: NULL}, {v:NULL},},
  {"pages/page", 32, &A2505, ft_binary, fc_stable, {b: FS_r_page}, {b: FS_w_page}, {v:NULL},},
};

DeviceEntry(8 B, DS1985U);

struct aggregate A2506 = { 256, ag_numbers, ag_separate, };
struct filetype DS2506[] = {
	F_STANDARD,
  {"memory", 8192, &A2506, ft_binary, fc_stable, {b: FS_r_memory}, {b: FS_w_memory}, {v:NULL},},
  {"pages", 0, NULL, ft_subdir, fc_volatile, {v: NULL}, {v: NULL}, {v:NULL},},
  {"pages/page", 32, NULL, ft_binary, fc_stable, {b: FS_r_page}, {b: FS_w_page}, {v:NULL},},
};

DeviceEntryExtended(0F, DS2506, DEV_ovdr);

struct filetype DS1986U[] = {
	F_STANDARD,
  {"memory", 8192, &A2506, ft_binary, fc_stable, {b: FS_r_memory}, {b: FS_w_memory}, {v:NULL},},
  {"pages", 0, NULL, ft_subdir, fc_volatile, {v: NULL}, {v: NULL}, {v:NULL},},
  {"pages/page", 32, NULL, ft_binary, fc_stable, {b: FS_r_page}, {b: FS_w_page}, {v:NULL},},
};

DeviceEntryExtended(8F, DS1986U, DEV_ovdr);

/* ------- Functions ------------ */

/* DS2505 */
static int OW_w_mem(const BYTE * data, const size_t size,
					const off_t offset, const struct parsedname *pn);
static int OW_r_mem(BYTE * data, const size_t size, const off_t offset,
					const struct parsedname *pn);

/* 2505 memory */
static int FS_r_memory(BYTE * buf, const size_t size, const off_t offset,
					   const struct parsedname *pn)
{
//    if ( OW_r_mem( buf, size, (size_t) offset, pn) ) return -EINVAL ;
	if (OW_read_paged(buf, size, (size_t) offset, pn, 32, OW_r_mem))
		return -EINVAL;
	return size;
}

static int FS_r_page(BYTE * buf, const size_t size, const off_t offset,
					 const struct parsedname *pn)
{
	if (OW_r_mem(buf, size, (size_t) (offset + (pn->extension << 5)), pn))
		return -EINVAL;
	return size;
}

static int FS_w_memory(const BYTE * buf, const size_t size,
					   const off_t offset, const struct parsedname *pn)
{
	if (OW_w_mem(buf, size, (size_t) offset, pn))
		return -EINVAL;
	return 0;
}

static int FS_w_page(const BYTE * buf, const size_t size,
					 const off_t offset, const struct parsedname *pn)
{
	if (OW_w_mem(buf, size, (size_t) (offset + (pn->extension << 5)), pn))
		return -EINVAL;
	return 0;
}

static int OW_w_mem(const BYTE * data, const size_t size,
					const off_t offset, const struct parsedname *pn)
{
	BYTE p[6] = { 0x0F, offset & 0xFF, offset >> 8, data[0] };
	int ret;
	struct transaction_log tfirst[] = {
		TRXN_START,
		{p, NULL, 4, trxn_match,},
		{NULL, &p[4], 2, trxn_read,},
		{p, NULL, 6, trxn_crc16,},
		{NULL, NULL, 0, trxn_program,},
		{NULL, p, 1, trxn_read,},
		TRXN_END,
	};

	if (size == 0)
		return 0;
	if (size == 1)
		return BUS_transaction(tfirst, pn) || (p[0] & (~data[0]));
	BUSLOCK(pn);
	if (BUS_transaction(tfirst, pn) || (p[0] & ~data[0])) {
		ret = 1;
	} else {
		size_t i;
		const BYTE *d = &data[1];
		UINT s = offset + 1;
		struct transaction_log trest[] = {
			{p, NULL, 1, trxn_match,},
			{NULL, &p[1], 2, trxn_read,},
			{p, (BYTE *) & s, 3, trxn_crc16seeded,},
			{NULL, NULL, 0, trxn_program,},
			{NULL, p, 1, trxn_read,},
			TRXN_END,
		};
		for (i = 0; i < size; ++i, ++d, ++s) {
			if (BUS_transaction(trest, pn) || (p[0] & ~d[0])) {
				ret = 1;
				break;
			}
		}
	}
	BUSUNLOCK(pn);
	return ret;
}

/* page oriented read -- call will not span pages */
static int OW_r_mem(BYTE * data, const size_t size, const off_t offset,
					const struct parsedname *pn)
{
	return OW_r_mem_crc16(data, size, offset, pn, 32);
}
