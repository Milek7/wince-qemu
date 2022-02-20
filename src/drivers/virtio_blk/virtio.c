#include <windows.h>
#include <types.h>
#include "ramdisk.h"

#define mmio_write16(reg, data)  *(volatile uint16_t*)(reg) = (data)
#define mmio_write32(reg, data)  *(volatile uint32_t*)(reg) = (data)
#define mmio_write64(reg, data)  *(volatile uint64_t*)(reg) = (data)
#define mmio_read16(reg)  *(volatile uint16_t*)(reg)
#define mmio_read32(reg)  *(volatile uint32_t*)(reg)
#define mmio_read64(reg)  *(volatile uint64_t*)(reg)

#define VIRTIO_ACK 1
#define VIRTIO_DRIVER 2
#define VIRTIO_FAILED 128
#define VIRTIO_FEATURES_OK 8
#define VIRTIO_DRIVER_OK 4
#define VIRTIO_DEVICE_NEEDS_RESET 64

#define VIRTIO_BLK_F_RO (1 << 5)
#define VIRTIO_BLK_F_FLUSH (1 << 9)
#define VIRTIO_BLK_F_DISCARD (1 << 13)

enum virtio_offsets {
    VIO_MagicValue = 0x0,
    VIO_Version    = 0x4,
    VIO_DeviceID   = 0x8,
    VIO_VendorID   = 0xc,

    VIO_DeviceFeatures         = 0x10,
    VIO_DeviceFeaturesSel      = 0x14,
    VIO_DriverFeatures         = 0x20,
    VIO_DriverFeaturesSel      = 0x24,

    VIO_QueueSel               = 0x30,
    VIO_QueueNumMax            = 0x34,
    VIO_QueueNum               = 0x38,
    VIO_QueueReady             = 0x44,
    VIO_QueueNotify            = 0x50,

    VIO_InterruptStatus        = 0x60,
    VIO_InterruptACK           = 0x64,

    VIO_Status                 = 0x70,

    VIO_QueueDescLow           = 0x80,
    VIO_QueueDescHigh          = 0x84,
    VIO_QueueDriverLow         = 0x90,
    VIO_QueueDriverHigh        = 0x94,
    VIO_QueueDeviceLow         = 0xa0,
    VIO_QueueDeviceHigh        = 0xa4,

    VIO_ConfigGeneration       = 0x0fc,
    VIO_ConfigOffset           = 0x100
};

#define VIRTIO_MAGIC 0x74726976
#define VIRTIO_DEVICEID_BLOCK 2

#define MAX_QUEUE_SIZE 256

BOOL InitVirtioBlk(PDISK pDisk)
{
	uint32_t iob = pDisk->ioBase;
	uint32_t dev_features, wanted_features, req_features;
	uint64_t sectors;

	if (mmio_read32(iob + VIO_MagicValue) != VIRTIO_MAGIC) {
		DEBUGMSG(ZONE_ERROR, (TEXT("virtio_blk: bad magic\r\n")));
        return FALSE;
	}

	if (mmio_read32(iob + VIO_Version) != 2) {
		DEBUGMSG(ZONE_ERROR, (TEXT("virtio_blk: bad version\r\n")));
        return FALSE;
	}

	if (mmio_read32(iob + VIO_DeviceID) != VIRTIO_DEVICEID_BLOCK) {
		DEBUGMSG(ZONE_ERROR, (TEXT("virtio_blk: bad type\r\n")));
        return FALSE;
	}

    mmio_write32(iob + VIO_Status, 0);
    mmio_write32(iob + VIO_Status, VIRTIO_ACK);
    mmio_write32(iob + VIO_Status, VIRTIO_ACK | VIRTIO_DRIVER);

    mmio_write32(iob + VIO_DeviceFeaturesSel, 0);
    mmio_write32(iob + VIO_DriverFeaturesSel, 0);

    dev_features = mmio_read32(iob + VIO_DeviceFeatures);
    wanted_features = VIRTIO_BLK_F_FLUSH | VIRTIO_BLK_F_RO | VIRTIO_F_EVENT_IDX;
    req_features = dev_features & wanted_features;

    mmio_write32(iob + VIO_DriverFeatures, req_features);

    mmio_write32(iob + VIO_Status, VIRTIO_ACK | VIRTIO_DRIVER | VIRTIO_FEATURES_OK);

    if (!(mmio_read32(iob + VIO_Status) & VIRTIO_FEATURES_OK)) {
        mmio_write32(iob + VIO_Status, VIRTIO_ACK | VIRTIO_DRIVER | VIRTIO_FAILED);
		DEBUGMSG(ZONE_ERROR, (TEXT("virtio_blk: bad features\r\n")));
        return FALSE;
    }

	DEBUGMSG(ZONE_INIT, (TEXT("virtio_blk: features: %x\r\n"), req_features));

	sectors = mmio_read64(iob + VIO_ConfigOffset);

	pDisk->vq_feat = req_features;
	pDisk->readonly = (pDisk->vq_feat & VIRTIO_BLK_F_RO) != 0;
	pDisk->sectors = (uint32_t)min(sectors, 0xFFFFFFFF);

    // initial setup done, init queue now
    {
		uint32_t queue_size_max;
		uint32_t desc_size, avail_size, used_size, headers_size, tails_size;
		uint32_t old_protect, pad, pad2, alloc_size;
		uint32_t phys_addr;

        mmio_write32(iob + VIO_QueueSel, 0);

        pDisk->vq.num = MAX_QUEUE_SIZE;
        queue_size_max = mmio_read32(iob + VIO_QueueNumMax);
        if (queue_size_max == 0) {
            mmio_write32(iob + VIO_Status, VIRTIO_ACK | VIRTIO_DRIVER | VIRTIO_FEATURES_OK | VIRTIO_FAILED);
			DEBUGMSG(ZONE_ERROR, (TEXT("virtio_blk: bad queue\r\n")));
            return FALSE;
        }
        if (pDisk->vq.num > queue_size_max)
            pDisk->vq.num = queue_size_max;
		pDisk->vq.num_mask = pDisk->vq.num - 1;

		DEBUGMSG(ZONE_INIT, (TEXT("virtio_blk: queue size: %u\r\n"), pDisk->vq.num));
        mmio_write32(iob + VIO_QueueNum, pDisk->vq.num);

        desc_size  = sizeof(struct virtq_desc) * pDisk->vq.num;
        avail_size = (sizeof(struct virtq_avail) + 2) + 2 * pDisk->vq.num;
        used_size  = (sizeof(struct virtq_used) + 2) + 8 * pDisk->vq.num;
        headers_size = sizeof(struct blk_header) * pDisk->vq.num;
        tails_size = sizeof(struct blk_tail) * pDisk->vq.num;

        pad = 0;
		pad2 = 0;
        if ((desc_size + avail_size) % 4)
            pad = 2;
        if ((desc_size + avail_size + pad + used_size) % 4)
            pad2 = 2;

        alloc_size = desc_size + avail_size + pad + used_size + pad2 + headers_size + tails_size;

        pDisk->virt_addr = AllocPhysMem(alloc_size, PAGE_NOACCESS, 0, 0, &phys_addr);
        if (!pDisk->virt_addr) {
            mmio_write32(iob + VIO_Status, VIRTIO_ACK | VIRTIO_DRIVER | VIRTIO_FEATURES_OK | VIRTIO_FAILED);
			DEBUGMSG(ZONE_ERROR, (TEXT("virtio_blk: AllocPhysMem failed\r\n")));
            return FALSE;
        }

        if (!VirtualProtect(pDisk->virt_addr, alloc_size, PAGE_READWRITE, &old_protect)) {
            mmio_write32(iob + VIO_Status, VIRTIO_ACK | VIRTIO_DRIVER | VIRTIO_FEATURES_OK | VIRTIO_FAILED);
			VirtualFree(pDisk->virt_addr, 0, MEM_RELEASE);
			DEBUGMSG(ZONE_ERROR, (TEXT("virtio_blk: VirtualProtect failed\r\n")));
            return FALSE;
        }

        mmio_write32(iob + VIO_QueueDescHigh, 0);
        mmio_write32(iob + VIO_QueueDescLow, phys_addr);

        mmio_write32(iob + VIO_QueueDriverHigh, 0);
        mmio_write32(iob + VIO_QueueDriverLow, phys_addr + desc_size);

        mmio_write32(iob + VIO_QueueDeviceHigh, 0);
        mmio_write32(iob + VIO_QueueDeviceLow, phys_addr + desc_size + avail_size + pad);

        mmio_write32(iob + VIO_QueueReady, 1);

		pDisk->vq.desc = (struct virtq_desc *)((uint32_t)pDisk->virt_addr);
		pDisk->vq.avail = (struct virtq_avail *)((uint32_t)pDisk->virt_addr + desc_size);
		pDisk->vq.used = (struct virtq_used *)((uint32_t)pDisk->virt_addr + desc_size + avail_size + pad);
		pDisk->vq.headers = (struct blk_header *)((uint32_t)pDisk->virt_addr + desc_size + avail_size + pad + used_size + pad2);
		pDisk->vq.tails = (struct blk_tail *)((uint32_t)pDisk->virt_addr + desc_size + avail_size + pad + used_size + pad2 + headers_size);

		pDisk->meta_phys_addr = phys_addr + desc_size + avail_size + pad + used_size + pad2;
    }

	pDisk->interrupt_ev = CreateEvent(NULL, FALSE, FALSE, NULL);

	if (!InterruptInitialize(pDisk->sysintr, pDisk->interrupt_ev, NULL, 0)) {
        mmio_write32(iob + VIO_Status, VIRTIO_ACK | VIRTIO_DRIVER | VIRTIO_FEATURES_OK | VIRTIO_FAILED);
		VirtualFree(pDisk->virt_addr, 0, MEM_RELEASE);
		CloseHandle(pDisk->interrupt_ev);
		DEBUGMSG(ZONE_ERROR, (TEXT("virtio_blk: InterruptInitialize failed\r\n")));
        return FALSE;
	}

    mmio_write32(iob + VIO_Status, VIRTIO_ACK | VIRTIO_DRIVER | VIRTIO_FEATURES_OK | VIRTIO_DRIVER_OK);

	// device ready
	return TRUE;
}

VOID CloseVirtioBlk(PDISK pDisk)
{
	mmio_write32(pDisk->ioBase + VIO_Status, 0);
	VirtualFree(pDisk->virt_addr, 0, MEM_RELEASE);
	if (WaitForSingleObject(pDisk->interrupt_ev, 0) != WAIT_TIMEOUT)
		InterruptDone(pDisk->sysintr);
	InterruptDisable(pDisk->sysintr);
	CloseHandle(pDisk->interrupt_ev);
}

//#define MAX_PFNS 32

#define PFN_SHIFT 0
#define PAGE_SIZE 0x1000
#define PAGE_MASK ~(PAGE_SIZE - 1)

VOID DoDiskIO(PDISK pDisk, DWORD Opcode, PSG_REQ pSgr)
{
	BOOL is_write = (Opcode == DISK_IOCTL_WRITE || Opcode == IOCTL_DISK_WRITE);
	uint32_t sectors_handled = 0;
	uint32_t current_sg = 0;
	uint32_t current_sg_pos = 0;
	uint32_t i, unlock_loop_end;
	//uint32_t sr_pfns[MAX_SG_BUF * MAX_PFNS];
	uint32_t *sr_pfns;
	uint32_t max_pfns = 0;
	BOOL ops_failure = FALSE;
	uint32_t request_bytes = 0;

	uint16_t *avail_ring = (uint16_t*)&pDisk->vq.avail[1];
	struct virtq_used_elem *used_ring = (struct virtq_used_elem*)&pDisk->vq.used[1];

	pSgr->sr_status = ERROR_IO_PENDING;

    if (pSgr->sr_num_sg > MAX_SG_BUF) {
		DEBUGMSG(ZONE_ERROR, (TEXT("virtio_blk: too many sg buffers\r\n")));
		pSgr->sr_status = ERROR_INVALID_PARAMETER;
		return;
	}

    if (pSgr->sr_start + pSgr->sr_num_sec > pDisk->sectors) {
		DEBUGMSG(ZONE_ERROR, (TEXT("virtio_blk: sectors out of bounds\r\n")));
        pSgr->sr_status = ERROR_SECTOR_NOT_FOUND;
		return;
	}

	if (is_write && pDisk->readonly) {
		DEBUGMSG(ZONE_ERROR, (TEXT("virtio_blk: device is read-only\r\n")));
		pSgr->sr_status = ERROR_WRITE_PROTECT;
		return;
	}

	for (i = 0; i < pSgr->sr_num_sg; i++) {
        char *data_ptr = pSgr->sr_sglist[i].sb_buf;
        char *data_pg = (char*)((uint32_t)data_ptr & PAGE_MASK);
        uint32_t data_pg_len = data_ptr + pSgr->sr_sglist[i].sb_len - data_pg;
		max_pfns = max(max_pfns, data_pg_len / PAGE_SIZE + 1);
	}

	sr_pfns = LocalAlloc(LMEM_FIXED, 4 * max_pfns * pSgr->sr_num_sg);

	for (i = 0; i < pSgr->sr_num_sg; i++) {
        char *data_ptr = pSgr->sr_sglist[i].sb_buf;
        char *data_pg = (char*)((uint32_t)data_ptr & PAGE_MASK);
        uint32_t data_pg_len = data_ptr + pSgr->sr_sglist[i].sb_len - data_pg;

		request_bytes += pSgr->sr_sglist[i].sb_len;

        if (!LockPages(data_pg, data_pg_len, &sr_pfns[i * max_pfns], LOCKFLAG_READ)) {
			DEBUGMSG(ZONE_ERROR, (TEXT("virtio_blk: LockPages failed\r\n")));
			pSgr->sr_status = ERROR_GEN_FAILURE;
			unlock_loop_end = i;
			goto unlock_pages;
        }
	}

	DEBUGMSG(ZONE_IO, (TEXT("virtio_blk: pages locked, request_bytes=%u, is_write=%u\r\n"), request_bytes, is_write));

	if (request_bytes != pSgr->sr_num_sec * 512) {
		DEBUGMSG(ZONE_ERROR, (TEXT("virtio_blk: sr buffers non-aligned to sector size\r\n")));
		pSgr->sr_status = ERROR_INVALID_PARAMETER;
		unlock_loop_end = pSgr->sr_num_sg;
		goto unlock_pages;
	}

	while (sectors_handled < pSgr->sr_num_sec) {
		uint16_t desc_pos = 0;
		uint16_t ops_counter = 0;
		uint16_t avail_idx, prev_avail_idx;

		EnterCriticalSection(&pDisk->d_DiskCardCrit);
		avail_idx = pDisk->vq.avail->idx;
		DEBUGMSG(ZONE_IO, (TEXT("virtio_blk: starting batch, %u sectors processed so far, idx %u\r\n"), sectors_handled, avail_idx));
		for (; sectors_handled < pSgr->sr_num_sec; ) {
			uint32_t sector_bytes_left = 512;

			DEBUGMSG(ZONE_IO, (TEXT("virtio_blk: filling header, idx=%u, desc_pos=%u\r\n"), avail_idx, desc_pos));
			avail_ring[(avail_idx++) & pDisk->vq.num_mask] = desc_pos;
			ops_counter++;
			pDisk->vq.desc[desc_pos].addr = pDisk->meta_phys_addr + sizeof(struct blk_header) * desc_pos;
			pDisk->vq.desc[desc_pos].len = 16;
			pDisk->vq.desc[desc_pos].flags = VIRTQ_DESC_F_NEXT;
			pDisk->vq.desc[desc_pos].next = desc_pos + 1;
			pDisk->vq.headers[desc_pos].type = is_write ? VIRTIO_BLK_T_OUT : VIRTIO_BLK_T_IN;
			pDisk->vq.headers[desc_pos].reserved = 0;
			pDisk->vq.headers[desc_pos].sector = pSgr->sr_start + sectors_handled;
			desc_pos++;

            while (sector_bytes_left > 0) {
				uint32_t sg_bytes_left, page_bytes_left, op_len;
				char *data_ptr, *data_pg, *data_startpg, *data_nextpg;

				data_startpg = (char*)((uint32_t)pSgr->sr_sglist[current_sg].sb_buf & PAGE_MASK);
				data_ptr = pSgr->sr_sglist[current_sg].sb_buf + current_sg_pos;
				data_pg = (char*)((uint32_t)data_ptr & PAGE_MASK);
				data_nextpg = data_pg + PAGE_SIZE;

				page_bytes_left = data_nextpg - data_ptr;
				sg_bytes_left = pSgr->sr_sglist[current_sg].sb_len - current_sg_pos;

				op_len = min(sector_bytes_left, min(sg_bytes_left, page_bytes_left));

				DEBUGMSG(ZONE_IO, (TEXT("virtio_blk: filling data, desc_pos=%u, op_len=%u, sector_bytes_left=%u, sg_bytes_left=%u, page_bytes_left=%u\r\n"), desc_pos, op_len, sector_bytes_left, sg_bytes_left, page_bytes_left));
				pDisk->vq.desc[desc_pos].addr = (sr_pfns[current_sg * max_pfns + (data_pg - data_startpg) / PAGE_SIZE] << PFN_SHIFT) | (data_ptr - data_pg);
				pDisk->vq.desc[desc_pos].len = op_len;
				pDisk->vq.desc[desc_pos].flags = VIRTQ_DESC_F_NEXT | (is_write ? 0 : VIRTQ_DESC_F_WRITE);
				pDisk->vq.desc[desc_pos].next = desc_pos + 1;
				desc_pos++;

				current_sg_pos += op_len;
				sector_bytes_left -= op_len;

                if (current_sg_pos == pSgr->sr_sglist[current_sg].sb_len) {
                    current_sg++;
					current_sg_pos = 0;

					if (current_sg == pSgr->sr_num_sg && (sector_bytes_left != 0 || (sectors_handled + 1 != pSgr->sr_num_sec))) {
						DEBUGMSG(ZONE_ERROR, (TEXT("virtio_blk: BUG! sr buffers too small for request\r\n")));
						pSgr->sr_num_sec = sectors_handled + 1;
						break;
					}
                }
            }

			DEBUGMSG(ZONE_IO, (TEXT("virtio_blk: filling tail, desc_pos=%u\r\n"), desc_pos));
			pDisk->vq.desc[desc_pos].addr = pDisk->meta_phys_addr + sizeof(struct blk_header) * pDisk->vq.num + sizeof(struct blk_tail) * desc_pos;
			pDisk->vq.desc[desc_pos].len = 1;
			pDisk->vq.desc[desc_pos].flags = VIRTQ_DESC_F_WRITE;
			pDisk->vq.desc[desc_pos].next = 0;
			pDisk->vq.tails[desc_pos].status = 0xFF;
			desc_pos++;

			sectors_handled++;

			if (desc_pos >= pDisk->vq.num - (MAX_SG_BUF * 2 + 2)) {
				DEBUGMSG(ZONE_IO, (TEXT("virtio_blk: filled %u descriptors, splitting request\r\n"), desc_pos));
				break;
			}
		}

		DEBUGMSG(ZONE_IO, (TEXT("virtio_blk: batch prepared, %u descriptors\r\n"), desc_pos));

		if (pDisk->vq_feat & VIRTIO_F_EVENT_IDX)
			*((uint16_t*)&avail_ring[pDisk->vq.num]) = avail_idx - 1;

		_dsb();

		prev_avail_idx = pDisk->vq.avail->idx;
		pDisk->vq.avail->idx = avail_idx;
		DEBUGMSG(ZONE_IO, (TEXT("virtio_blk: advancing avail_idx: %u\r\n"), avail_idx));

		_dsb();

		if (pDisk->vq_feat & VIRTIO_F_EVENT_IDX) {
			uint16_t avail_event = *((uint16_t*)&used_ring[pDisk->vq.num]);
			if (VIRTQ_NEED_EVENT(avail_event, avail_idx, prev_avail_idx))
				mmio_write32(pDisk->ioBase + VIO_QueueNotify, 0);
		} else {
			if (!(pDisk->vq.used->flags & VIRTQ_USED_F_NO_NOTIFY))
				mmio_write32(pDisk->ioBase + VIO_QueueNotify, 0);
		}

		while (ops_counter > 0) {
			if (WaitForSingleObject(pDisk->interrupt_ev, 1000) == WAIT_OBJECT_0) {
				mmio_write32(pDisk->ioBase + VIO_InterruptACK, mmio_read32(pDisk->ioBase + VIO_InterruptStatus));
				InterruptDone(pDisk->sysintr);
			} else {
				DEBUGMSG(ZONE_WARNING, (TEXT("virtio_blk: interrupt timed out\r\n")));
				mmio_write32(pDisk->ioBase + VIO_QueueNotify, 0);
			}

			_dsb();

			while (pDisk->last_used_idx != pDisk->vq.used->idx) {
				uint32_t bytes_left = 512 + 16;
				uint16_t pos;
				uint8_t status;

				pos = used_ring[pDisk->last_used_idx & pDisk->vq.num_mask].id;
				DEBUGMSG(ZONE_IO, (TEXT("virtio_blk: checking used idx=%u, desc_pos=%u\r\n"), pDisk->last_used_idx, pos));

				while (bytes_left != 0) {
					bytes_left -= pDisk->vq.desc[pos].len;
					pos = pDisk->vq.desc[pos].next;
				}

				status = pDisk->vq.tails[pos].status;

				if (status != VIRTIO_BLK_S_OK) {
					DEBUGMSG(ZONE_ERROR, (TEXT("virtio_blk: io failed, idx=%u, status_pos=%u, status=%x\r\n"), pDisk->last_used_idx, pos, status));
					ops_failure = TRUE;
				}

				ops_counter--;
				pDisk->last_used_idx++;
			}
		}

		LeaveCriticalSection(&pDisk->d_DiskCardCrit);
	}

	if (current_sg != pSgr->sr_num_sg) {
		DEBUGMSG(ZONE_ERROR, (TEXT("virtio_blk: BUG! sr buffers too big for request\r\n")));
	}

	pSgr->sr_status = ops_failure ? (is_write ? ERROR_WRITE_FAULT : ERROR_READ_FAULT) : ERROR_SUCCESS;

	unlock_loop_end = pSgr->sr_num_sg;
unlock_pages:
	for (i = 0; i < unlock_loop_end; i++) {
        char *data_ptr = pSgr->sr_sglist[i].sb_buf;
        char *data_pg = (char*)((uint32_t)data_ptr & PAGE_MASK);
        uint32_t data_pg_len = data_ptr + pSgr->sr_sglist[i].sb_len - data_pg;

        if (!UnlockPages(data_pg, data_pg_len)) {
			DEBUGMSG(ZONE_ERROR, (TEXT("virtio_blk: UnlockPages failed\r\n")));
			pSgr->sr_status = ERROR_GEN_FAILURE;
            return;
        }
	}

	LocalFree(sr_pfns);

	DEBUGMSG(ZONE_IO, (TEXT("virtio_blk: pages unlocked, IO finished\r\n")));
}
