
// http://www.simtec.co.uk/products/SWLINUX/files/booting_article.html#appendix_tag_reference
#include "atag.h"

#define tag_next(t)	   ((struct atag *)((u32 *)(t) + (t)->hdr.size))
#define tag_size(type) ((sizeof(struct atag_header) + sizeof(struct type)) >> 2)
static struct atag *params; /* used to point at the current tag */

void setup_core_tag(void *address, long pagesize)
{
	params = (struct atag *)address; /* Initialise parameters to start at given address */

	params->hdr.tag	 = ATAG_CORE; /* start with the core tag */
	params->hdr.size = tag_size(atag_core); /* size the tag */

	params->u.core.flags	= 1; /* ensure read-only */
	params->u.core.pagesize = pagesize; /* systems pagesize (4k) */
	params->u.core.rootdev	= 0; /* zero root device (typicaly overidden from commandline )*/

	params = tag_next(params); /* move pointer to next tag */
}

void setup_ramdisk_tag(u32 size)
{
	params->hdr.tag	 = ATAG_RAMDISK; /* Ramdisk tag */
	params->hdr.size = tag_size(atag_ramdisk); /* size tag */

	params->u.ramdisk.flags = 0; /* Load the ramdisk */
	params->u.ramdisk.size	= size; /* Decompressed ramdisk size */
	params->u.ramdisk.start = 0; /* Unused */

	params = tag_next(params); /* move pointer to next tag */
}

void setup_initrd2_tag(u32 start, u32 size)
{
	params->hdr.tag	 = ATAG_INITRD2; /* Initrd2 tag */
	params->hdr.size = tag_size(atag_initrd2); /* size tag */

	params->u.initrd2.start = start; /* physical start */
	params->u.initrd2.size	= size; /* compressed ramdisk size */

	params = tag_next(params); /* move pointer to next tag */
}

void setup_mem_tag(u32 start, u32 len)
{
	params->hdr.tag	 = ATAG_MEM; /* Memory tag */
	params->hdr.size = tag_size(atag_mem); /* size tag */

	params->u.mem.start = start; /* Start of memory area (physical address) */
	params->u.mem.size	= len; /* Length of area */

	params = tag_next(params); /* move pointer to next tag */
}

void setup_cmdline_tag(const char *line)
{
	int linelen = strlen(line);

	if (!linelen)
		return; /* do not insert a tag for an empty commandline */

	params->hdr.tag	 = ATAG_CMDLINE; /* Commandline tag */
	params->hdr.size = (sizeof(struct atag_header) + linelen + 1 + 4) >> 2;

	strcpy(params->u.cmdline.cmdline, line); /* place commandline into tag */

	params = tag_next(params); /* move pointer to next tag */
}

void setup_end_tag(void)
{
	params->hdr.tag	 = ATAG_NONE; /* Empty tag ends list */
	params->hdr.size = 0; /* zero length */
}
