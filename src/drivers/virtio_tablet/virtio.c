#include <windows.h>
#include <types.h>
#include "virtio_queue.h"

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

#define ZONE_INIT       DEBUGZONE(0)
#define ZONE_ERROR      DEBUGZONE(1)
#define ZONE_WARN       DEBUGZONE(2)
#define ZONE_IO         DEBUGZONE(3)
#define ZONE_INFO       DEBUGZONE(4)

#define VIRTIO_MAGIC 0x74726976
#define VIRTIO_DEVICEID_INPUT 18

#define MAX_QUEUE_SIZE 256

struct virtio_cfg
{
	uint32_t ioBase;
	uint32_t sysintr;
	HANDLE interrupt_ev;
	uint32_t vq_feat;

	struct virtq vq[2];
};

BOOL InitVirtioInput(struct virtio_cfg *cfg)
{
	uint32_t i;
	uint32_t iob = cfg->ioBase;
	uint32_t dev_features, wanted_features, req_features;

	if (mmio_read32(iob + VIO_MagicValue) != VIRTIO_MAGIC) {
		DEBUGMSG(ZONE_ERROR, (TEXT("virtio_input: bad magic\r\n")));
        return FALSE;
	}

	if (mmio_read32(iob + VIO_Version) != 2) {
		DEBUGMSG(ZONE_ERROR, (TEXT("virtio_input: bad version\r\n")));
        return FALSE;
	}

	if (mmio_read32(iob + VIO_DeviceID) != VIRTIO_DEVICEID_INPUT) {
		DEBUGMSG(ZONE_ERROR, (TEXT("virtio_input: bad type\r\n")));
        return FALSE;
	}

    mmio_write32(iob + VIO_Status, 0);
    mmio_write32(iob + VIO_Status, VIRTIO_ACK);
    mmio_write32(iob + VIO_Status, VIRTIO_ACK | VIRTIO_DRIVER);

    mmio_write32(iob + VIO_DeviceFeaturesSel, 0);
    mmio_write32(iob + VIO_DriverFeaturesSel, 0);

    dev_features = mmio_read32(iob + VIO_DeviceFeatures);
    wanted_features = VIRTIO_F_EVENT_IDX;
    req_features = dev_features & wanted_features;

    mmio_write32(iob + VIO_DriverFeatures, req_features);

    mmio_write32(iob + VIO_Status, VIRTIO_ACK | VIRTIO_DRIVER | VIRTIO_FEATURES_OK);

    if (!(mmio_read32(iob + VIO_Status) & VIRTIO_FEATURES_OK)) {
        mmio_write32(iob + VIO_Status, VIRTIO_ACK | VIRTIO_DRIVER | VIRTIO_FAILED);
		DEBUGMSG(ZONE_ERROR, (TEXT("virtio_input: bad features\r\n")));
        return FALSE;
    }

	DEBUGMSG(ZONE_INIT, (TEXT("virtio_input: features: %x\r\n"), req_features));

	cfg->vq_feat = req_features;

    // initial setup done, init queues now
	for (i = 0; i < 2; i++)
    {
		uint32_t queue_size_max;
		uint32_t desc_size, avail_size, used_size, events_size;
		uint32_t old_protect, pad, pad2, alloc_size;
		uint32_t phys_addr;

        mmio_write32(iob + VIO_QueueSel, i);

        cfg->vq[i].num = MAX_QUEUE_SIZE;
        queue_size_max = mmio_read32(iob + VIO_QueueNumMax);
        if (queue_size_max == 0) {
            mmio_write32(iob + VIO_Status, VIRTIO_ACK | VIRTIO_DRIVER | VIRTIO_FEATURES_OK | VIRTIO_FAILED);
			DEBUGMSG(ZONE_ERROR, (TEXT("virtio_input: bad queue\r\n")));
            return FALSE;
        }
        if (cfg->vq[i].num > queue_size_max)
            cfg->vq[i].num = queue_size_max;
		cfg->vq[i].num_mask = cfg->vq[i].num - 1;

		DEBUGMSG(ZONE_INIT, (TEXT("virtio_input: queue size: %u\r\n"), cfg->vq[i].num));
        mmio_write32(iob + VIO_QueueNum, cfg->vq[i].num);

        desc_size  = sizeof(struct virtq_desc) * cfg->vq[i].num;
        avail_size = (sizeof(struct virtq_avail) + 2) + 2 * cfg->vq[i].num;
        used_size  = (sizeof(struct virtq_used) + 2) + 8 * cfg->vq[i].num;
        events_size = sizeof(struct virtio_input_event) * cfg->vq[i].num;

        pad = 0;
		pad2 = 0;
        if ((desc_size + avail_size) % 4)
            pad = 2;
        if ((desc_size + avail_size + pad + used_size) % 4)
            pad2 = 2;

        alloc_size = desc_size + avail_size + pad + used_size + pad2 + events_size;

        cfg->vq[i].virt_addr = AllocPhysMem(alloc_size, PAGE_NOACCESS, 0, 0, &phys_addr);
        if (!cfg->vq[i].virt_addr) {
            mmio_write32(iob + VIO_Status, VIRTIO_ACK | VIRTIO_DRIVER | VIRTIO_FEATURES_OK | VIRTIO_FAILED);
			DEBUGMSG(ZONE_ERROR, (TEXT("virtio_input: AllocPhysMem failed\r\n")));
            return FALSE;
        }

        if (!VirtualProtect(cfg->vq[i].virt_addr, alloc_size, PAGE_READWRITE, &old_protect)) {
            mmio_write32(iob + VIO_Status, VIRTIO_ACK | VIRTIO_DRIVER | VIRTIO_FEATURES_OK | VIRTIO_FAILED);
			VirtualFree(cfg->vq[i].virt_addr, 0, MEM_RELEASE);
			DEBUGMSG(ZONE_ERROR, (TEXT("virtio_input: VirtualProtect failed\r\n")));
            return FALSE;
        }

        mmio_write32(iob + VIO_QueueDescHigh, 0);
        mmio_write32(iob + VIO_QueueDescLow, phys_addr);

        mmio_write32(iob + VIO_QueueDriverHigh, 0);
        mmio_write32(iob + VIO_QueueDriverLow, phys_addr + desc_size);

        mmio_write32(iob + VIO_QueueDeviceHigh, 0);
        mmio_write32(iob + VIO_QueueDeviceLow, phys_addr + desc_size + avail_size + pad);

        mmio_write32(iob + VIO_QueueReady, 1);

		cfg->vq[i].desc = (struct virtq_desc *)((uint32_t)cfg->vq[i].virt_addr);
		cfg->vq[i].avail = (struct virtq_avail *)((uint32_t)cfg->vq[i].virt_addr + desc_size);
		cfg->vq[i].used = (struct virtq_used *)((uint32_t)cfg->vq[i].virt_addr + desc_size + avail_size + pad);
		cfg->vq[i].events = (struct virtio_input_event *)((uint32_t)cfg->vq[i].virt_addr + desc_size + avail_size + pad + used_size + pad2);

		cfg->vq[i].meta_phys_addr = phys_addr + desc_size + avail_size + pad + used_size + pad2;
    }

	cfg->interrupt_ev = CreateEvent(NULL, FALSE, FALSE, NULL);

	if (!InterruptInitialize(cfg->sysintr, cfg->interrupt_ev, NULL, 0)) {
        mmio_write32(iob + VIO_Status, VIRTIO_ACK | VIRTIO_DRIVER | VIRTIO_FEATURES_OK | VIRTIO_FAILED);
		VirtualFree(cfg->vq[0].virt_addr, 0, MEM_RELEASE);
		VirtualFree(cfg->vq[1].virt_addr, 0, MEM_RELEASE);
		CloseHandle(cfg->interrupt_ev);
		DEBUGMSG(ZONE_ERROR, (TEXT("virtio_input: InterruptInitialize failed\r\n")));
        return FALSE;
	}

    mmio_write32(iob + VIO_Status, VIRTIO_ACK | VIRTIO_DRIVER | VIRTIO_FEATURES_OK | VIRTIO_DRIVER_OK);

	// device ready
	return TRUE;
}

VOID CloseVirtioInput(struct virtio_cfg *cfg)
{
	mmio_write32(cfg->ioBase + VIO_Status, 0);
	VirtualFree(cfg->vq[0].virt_addr, 0, MEM_RELEASE);
	VirtualFree(cfg->vq[1].virt_addr, 0, MEM_RELEASE);
	if (WaitForSingleObject(cfg->interrupt_ev, 0) != WAIT_TIMEOUT)
		InterruptDone(cfg->sysintr);
	InterruptDisable(cfg->sysintr);
	CloseHandle(cfg->interrupt_ev);
}

struct virtio_cfg cfginst;

BOOL init(void)
{
	cfginst.ioBase = 0x91130600;
	cfginst.sysintr = 0x15;
	return InitVirtioInput(&cfginst);
}

void close(void)
{
	CloseVirtioInput(&cfginst);
}

void _dsb(void);

#define EV_SYN			0x00
#define EV_KEY			0x01
#define EV_REL			0x02
#define EV_ABS			0x03

#define SYN_REPORT		0

#define ABS_X			0x00
#define ABS_Y			0x01

#define REL_WHEEL		0x08

#define BTN_MOUSE		0x110
#define BTN_LEFT		0x110
#define BTN_RIGHT		0x111
#define BTN_MIDDLE		0x112

struct input_state
{
	int32_t x;
	int32_t y;
};

struct input_state input_buf;

void process_event(struct virtio_input_event *ev)
{
	//mouse_event(MOUSEEVENTF_MOVE | MOUSEEVENTF_TOUCH, (long long)input_buf.x * 1024 / (32768 / 4), (long long)input_buf.y * 600 / (32768 / 4), 0, 0);

	if (ev->type == EV_SYN) {
		if (ev->code == SYN_REPORT)
			mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE, input_buf.x * 2, input_buf.y * 2, 0, 0);
	} else if (ev->type == EV_ABS) {
		if (ev->code == ABS_X)
			input_buf.x = ev->value;
		if (ev->code == ABS_Y)
			input_buf.y = ev->value;
	} else if (ev->type == EV_KEY) {
		if (ev->code == BTN_LEFT && ev->value == 0)
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
		if (ev->code == BTN_LEFT && ev->value == 1)
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);

		if (ev->code == BTN_RIGHT && ev->value == 0)
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
		if (ev->code == BTN_RIGHT && ev->value == 1)
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);

		if (ev->code == BTN_MIDDLE && ev->value == 0)
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, 0);
		if (ev->code == BTN_MIDDLE && ev->value == 1)
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, 0);
	} else if (ev->type == EV_REL) {
		if (ev->code == REL_WHEEL)
			mouse_event(MOUSEEVENTF_WHEEL, 0, 0, ev->value * WHEEL_DELTA, 0);
	}
}

void populate_buffers(struct virtio_cfg *cfg)
{
	uint16_t desc_pos, avail_idx, prev_avail_idx;
	struct virtq *vq = &cfg->vq[0];

	uint16_t *avail_ring = (uint16_t*)&vq->avail[1];
	struct virtq_used_elem *used_ring = (struct virtq_used_elem*)&vq->used[1];

	DEBUGMSG(ZONE_IO, (TEXT("virtio_input: filling\r\n")));

	avail_idx = vq->avail->idx;
	while (((vq->next_fill_pos + 1) & vq->num_mask) != (vq->last_used_idx & vq->num_mask))
	{
		desc_pos = (vq->next_fill_pos++) & vq->num_mask;
		DEBUGMSG(ZONE_IO, (TEXT("virtio_input: filling event, idx=%u, desc_pos=%u\r\n"), avail_idx, desc_pos));
		avail_ring[(avail_idx++) & vq->num_mask] = desc_pos;
		vq->desc[desc_pos].addr = vq->meta_phys_addr + sizeof(struct virtio_input_event) * desc_pos;
		vq->desc[desc_pos].len = sizeof(struct virtio_input_event);
		vq->desc[desc_pos].flags = VIRTQ_DESC_F_WRITE;
		vq->desc[desc_pos].next = 0;
		vq->events[desc_pos].type = 0;
		vq->events[desc_pos].code = 0;
		vq->events[desc_pos].value = 0;
	}

	if (cfg->vq_feat & VIRTIO_F_EVENT_IDX)
		*((uint16_t*)&avail_ring[vq->num]) = vq->last_used_idx;

	_dsb();

	prev_avail_idx = vq->avail->idx;
	vq->avail->idx = avail_idx;
	DEBUGMSG(ZONE_IO, (TEXT("virtio_input: advancing avail_idx: %u\r\n"), avail_idx));

	_dsb();

	if (cfg->vq_feat & VIRTIO_F_EVENT_IDX) {
		uint16_t avail_event = *((uint16_t*)&used_ring[vq->num]);
		if (VIRTQ_NEED_EVENT(avail_event, avail_idx, prev_avail_idx))
			mmio_write32(cfg->ioBase + VIO_QueueNotify, 0);
	} else {
		if (!(vq->used->flags & VIRTQ_USED_F_NO_NOTIFY))
			mmio_write32(cfg->ioBase + VIO_QueueNotify, 0);
	}
}

void drain_buffers(struct virtio_cfg *cfg)
{
	struct virtq *vq = &cfg->vq[0];

	uint16_t *avail_ring = (uint16_t*)&vq->avail[1];
	struct virtq_used_elem *used_ring = (struct virtq_used_elem*)&vq->used[1];

	if (WaitForSingleObject(cfg->interrupt_ev, 1000) == WAIT_OBJECT_0) {
		mmio_write32(cfg->ioBase + VIO_InterruptACK, mmio_read32(cfg->ioBase + VIO_InterruptStatus));
		InterruptDone(cfg->sysintr);
	} else {
		mmio_write32(cfg->ioBase + VIO_QueueNotify, 0);
	}

	_dsb();

again:
	while (vq->last_used_idx != vq->used->idx) {
		uint16_t pos;
		struct virtio_input_event *ev;

		pos = used_ring[vq->last_used_idx & vq->num_mask].id;
		ev = &vq->events[pos];
		DEBUGMSG(ZONE_IO, (TEXT("virtio_input: idx=%u, desc_pos=%u\r\n"), vq->last_used_idx, pos));
		process_event(ev);

		vq->last_used_idx++;
	}

	if (cfg->vq_feat & VIRTIO_F_EVENT_IDX)
		*((uint16_t*)&avail_ring[vq->num]) = vq->last_used_idx;

	_dsb();

	if (vq->last_used_idx != vq->used->idx)
		goto again;
}

void Poll(struct virtio_cfg *cfg)
{
	populate_buffers(cfg);
	drain_buffers(cfg);
}

void poll(void)
{
	Poll(&cfginst);
}