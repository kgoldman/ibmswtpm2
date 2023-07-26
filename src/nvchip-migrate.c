/* the incompatible rev1332 sizes for this */
#define NV_STATE_RESET_DATA_1332	0x0428
#define SIZE_STATE_RESET_DATA_1332	0x0188
#define NV_STATE_CLEAR_DATA_1332	0x05b0
#define SIZE_STATE_CLEAR_DATA_1332	0x0b1c
#define NV_ORDERLY_DATA_1332		0x10cc
#define SIZE_ORDERLY_DATA_1332		0x0078
#define NV_INDEX_RAM_DATA_1332		0x1144
#define SIZE_INDEX_RAM_DATA_1332	0x0200
#define NV_USER_DYNAMIC_1332		0x1344

#define OBJECT_SIZE_1563		0x0768
#define PUBLIC_AREA_SIZE_1563		0x0164
#define SENSITIVE_SIZE_1563		0x0308
#define SENSITIVE_OFFSET_1563		0x0168
#define QUALIFIED_NAME_OFFSET_1563	0x06d0

#define NV_C
#include "Tpm.h"
#include "Platform.h"

struct end_marker {
	UINT32	zero;
	UINT64  maxcount;
} __attribute__((packed));

struct nv_header {
	UINT32 size;
	UINT32 handle;
};

struct end_marker EM;

/* The state changed size in the next revision after 1336 */
static void
migrate_1332(void)
{
	/* move up INDEX_RAM_DATA + USER_DYNAMIC */
	memmove(s_NV + NV_INDEX_RAM_DATA, s_NV + NV_INDEX_RAM_DATA_1332,
		NV_USER_DYNAMIC_END - NV_INDEX_RAM_DATA);
	/* move up ORDERLY_DATA with expansion */
	memmove(s_NV + NV_ORDERLY_DATA, s_NV + NV_ORDERLY_DATA_1332,
		SIZE_ORDERLY_DATA_1332);
	/* zero the rest */
	memset(s_NV + NV_ORDERLY_DATA + SIZE_ORDERLY_DATA_1332, 0,
	       sizeof(ORDERLY_DATA) - SIZE_ORDERLY_DATA_1332);
	/* move up STATE_CLEAR_DATA with expansion */
	memmove(s_NV + NV_STATE_CLEAR_DATA, s_NV + NV_STATE_CLEAR_DATA_1332,
		SIZE_STATE_CLEAR_DATA_1332);
	/* zero the rest */
	memset(s_NV + NV_STATE_CLEAR_DATA + SIZE_STATE_CLEAR_DATA_1332, 0,
	       sizeof(STATE_CLEAR_DATA) - SIZE_STATE_CLEAR_DATA_1332);
	/* move up STATE_RESET_DATA with expansion */
	memmove(s_NV + NV_STATE_RESET_DATA, s_NV + NV_STATE_RESET_DATA_1332,
		SIZE_STATE_RESET_DATA_1332);
	/* zero the rest */
	memset(s_NV + NV_STATE_RESET_DATA + SIZE_STATE_RESET_DATA_1332, 0,
	       sizeof(STATE_RESET_DATA) - SIZE_STATE_RESET_DATA_1332);
}

static void
migrate_object_1563(int offset)
{
	BYTE *o = s_NV + offset;
	struct nv_header *h = (struct nv_header *)o;

	o += sizeof(*h);
	OBJECT *obj = (OBJECT *)o;

	/* before doing anything, move every other object after this
	 * to make room for this as a new object size */
	memmove(o + sizeof(OBJECT),
		o + OBJECT_SIZE_1563,
		NV_USER_DYNAMIC_END - (o - s_NV) - sizeof(OBJECT));

	/* update the object to its new size */
	h->size = sizeof(*obj) + sizeof(*h);

	/* move up the qualifiedName, evictHandle and name */
	memmove(&obj->qualifiedName,
		o + QUALIFIED_NAME_OFFSET_1563,
		OBJECT_SIZE_1563 - QUALIFIED_NAME_OFFSET_1563);

	/* ignore the privateExponent; we'll recreate it */

	/* move up the sensitive */
	memmove(&obj->sensitive,
		o + SENSITIVE_OFFSET_1563,
		SENSITIVE_SIZE_1563);

	/* now recreate the privateExponent */
	if (obj->publicArea.type == TPM_ALG_RSA)
		CryptRsaLoadPrivateExponent(obj);

}

/* the dynamic OBJECT size changed after revision 1563.  The dynamic
 * area must be validated by check_nv() before calling this */
static void
migrate_dynamic_1563(int offset)
{
	struct nv_header *h;

	for (;;) {
		if (memcmp(&EM, s_NV + offset, sizeof(EM)) == 0)
			return;
		h = (struct nv_header *)(s_NV + offset);
		if ((h->handle & 0xff000000) == 0x81000000) {
			if (h->size == OBJECT_SIZE_1563 + sizeof(*h)) {
				fprintf(stderr, "migrating object %08x\n",
					h->handle);
				migrate_object_1563(offset);
				/* re-read */
				h = (struct nv_header *)(s_NV + offset);
			}
			if (h->size != sizeof(OBJECT) + sizeof(*h)) {
				fprintf(stderr, "NVChip dynamic area has invalid object size %d\n", h->size);
				exit(1);
			}
		}
		offset += h->size;
	}
}

static int
check_nv(int offset)
{
	struct nv_header *h;
	for (;;) {
		if (memcmp(&EM, s_NV + offset, sizeof(EM)) == 0)
			return 1;
		h = (struct nv_header *)(s_NV + offset);

		if ((h->handle & 0xff000000) != 0x01000000 &&
		    (h->handle & 0xff000000) != 0x81000000)
			/* MSO must be 01 or 81 */
			return 0;
		if (h->size + offset >= NV_USER_DYNAMIC_END)
			/* size is wrong */
			return 0;
		if (h->size == 0)
			/* zero size must be the end marker */
			return 0;
		offset += h->size;
	}
}

void
validate_nvchip(void)
{
	int i;
	struct end_marker *m;

	/* First search backwards from the end of the NVChip file.  All the
	 * words should be 0xffffffff until we hit the end marker.
	 *
	 * We assume that maxcount hasn't grown so big as to be about to
	 * overflow */
	for (i = NV_USER_DYNAMIC_END - sizeof(UINT32); i >= 0;
	     i -= sizeof(UINT32)) {
		m = (struct end_marker *)(s_NV + i);
		if (m->zero != 0xffffffff) {
			/* we found the first non 0xffff, so the end
			   marker should be another two words back */
			i -= 2*sizeof(UINT32);
			m = (struct end_marker *)(s_NV + i);

			if (m->zero != 0) {
				fprintf(stderr, "NVChip file has invalid end marker value 0x%x @0x%x\n", m->zero, i);
				exit (1);
			}
			break;
		}
	}

	/* now take a copy of the end marker pattern */
	EM = *m;

	/* now check the current layout */
	if (check_nv(NV_USER_DYNAMIC)) {
		printf("NVChip is current\n");
		goto migrate_object;
	}
	if (check_nv(NV_USER_DYNAMIC_1332)) {
		fprintf(stderr, "NVChip is older (1332) version\n");
		migrate_1332();
		if (check_nv(NV_USER_DYNAMIC)) {
			fprintf(stderr, "NVChip successfully migrated\n");
			goto migrate_object;
		}
		fprintf(stderr, "Failed to migrate NVChip from 1332 format\n");
		exit(1);
	}
	fprintf(stderr, "Unknown NVChip layout\n");
	exit(1);
 migrate_object:
	migrate_dynamic_1563(NV_USER_DYNAMIC);
}
